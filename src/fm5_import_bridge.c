#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "graphics-port/fm5_graphics_bridge.h"
#include "audio-tls/fm5_audio_bridge.h"
#include "fm5_activation.h"
#include "audio-tls/apu-port/fm5_apu.h"
#include "graphics-port/fm5_presentation.h"
#include "graphics-port/fm5_color_observer.h"
#include "file-port/fm5_scene_files.h"

typedef struct ImportString {
    union { char small[16]; const char *pointer; } storage;
    size_t length;
    size_t capacity;
} ImportString;

typedef struct ImportRecord {
    ImportString library;
    ImportString symbol;
    uint32_t iat_rva;
    uint32_t padding;
    void *resolved;
} ImportRecord;

typedef FARPROC (*ResolveImport)(const ImportRecord *);
int FM5RunApuNativeProbe(void);
static HMODULE xg_module;
static SRWLOCK xg_lock = SRWLOCK_INIT;

static const char *string_data(const ImportString *value) {
    return value->capacity > 15 ? value->storage.pointer : value->storage.small;
}

static int string_is(const ImportString *value, const char *expected) {
    size_t size = strlen(expected);
    return value->length == size && memcmp(string_data(value), expected, size) == 0;
}

static int module_directory(wchar_t *path, DWORD capacity) {
    HMODULE module = NULL;
    DWORD length;
    wchar_t *slash;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                           GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                           (LPCWSTR)(uintptr_t)&module_directory, &module)) return 0;
    length = GetModuleFileNameW(module, path, capacity);
    if (!length || length >= capacity) return 0;
    slash = wcsrchr(path, L'\\');
    if (!slash) return 0;
    slash[1] = 0;
    return 1;
}

static void trace_import(const ImportRecord *record, const char *route, FARPROC target) {
    wchar_t path[32768];
    char line[768];
    int length;
    HANDLE file;
    DWORD written;
    if (!module_directory(path, 32768) ||
        wcscat_s(path, 32768, L"fm5-imports.log") != 0) return;
    length = _snprintf_s(line, sizeof(line), _TRUNCATE,
        "%llu %.*s!%.*s iat=%08lx route=%s target=%p\r\n",
        (unsigned long long)GetTickCount64(),
        (int)record->library.length, string_data(&record->library),
        (int)record->symbol.length, string_data(&record->symbol),
        (unsigned long)record->iat_rva, route, (void *)target);
    if (length <= 0) return;
    file = CreateFileW(path, FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return;
    WriteFile(file, line, (DWORD)length, &written, NULL);
    CloseHandle(file);
}

static HMODULE load_original_xg(void) {
    wchar_t path[32768];
    HMODULE result;
    AcquireSRWLockExclusive(&xg_lock);
    if (!xg_module) {
        static const wchar_t *relative[] = {L"xg-host\\xg.dll", L"..\\xg-host\\xg.dll"};
        for (UINT index = 0; index < ARRAYSIZE(relative) && !xg_module; ++index)
            if (module_directory(path, 32768) && wcscat_s(path, 32768, relative[index]) == 0)
                xg_module = LoadLibraryExW(path, NULL,
                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    }
    result = xg_module;
    ReleaseSRWLockExclusive(&xg_lock);
    return result;
}

static int known_azimuth_host(const unsigned char *base) {
    const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)base;
    const IMAGE_NT_HEADERS64 *nt;
    if (!base || dos->e_magic != IMAGE_DOS_SIGNATURE ||
        dos->e_lfanew <= 0 || dos->e_lfanew > 4096) return 0;
    nt = (const IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    return nt->Signature == IMAGE_NT_SIGNATURE &&
           nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
           nt->OptionalHeader.AddressOfEntryPoint == 0x2226fc &&
           nt->OptionalHeader.SizeOfImage >= 0x3b9000 &&
           (nt->OptionalHeader.SizeOfImage <= 0x3c9000 ||
            nt->OptionalHeader.SizeOfImage == 0x3de000);
}

typedef struct NativeImage {
    unsigned char *base;
    uint32_t size;
} NativeImage;

typedef struct NativeOptions {
    uint32_t unknown;
    uint32_t height;
    uint32_t width;
    uint32_t frame_rate;
    unsigned char real_time;
    unsigned char padding[3];
    uint32_t graphics_groups[6];
    unsigned char ui_clock;
    unsigned char padding2[3];
    void *optional_value;
} NativeOptions;

HRESULT WINAPI FM5GetLegacyTexture2DDescription(void *, void *);
typedef void (WINAPI *FM5UiGetResource)(void *, void **);
typedef ULONG (WINAPI *FM5UiRelease)(void *);
typedef void (WINAPI *FM5UiGetTargets)(void *, UINT, void **, void **);
typedef void *(*FM5UiAllocateTarget)(void *, UINT, UINT);
typedef void (*FM5UiCopyTarget)(void *, void *, const float *);
static FM5UiAllocateTarget fm5_ui_allocate_target;
static FM5UiCopyTarget fm5_ui_copy_target;

__declspec(noreturn) static void fm5_ui_failure(HRESULT status, const void *object) {
    ULONG_PTR details[] = {(ULONG_PTR)(uint32_t)status, (ULONG_PTR)object};
    RaiseException(0xe0f5000c, EXCEPTION_NONCONTINUABLE, 2, details);
    ExitProcess(ERROR_INVALID_DATA);
}

static HRESULT fm5_ui_view_extent(void *view, UINT *width, UINT *height) {
    void *resource = NULL;
    DWORD description[12] = {0};
    HRESULT status;
    if (!view) return E_POINTER;
    /* View.GetResource is slot8 in both pinned SDKs. It owns one reference;
       Texture2D's old11/native12 GetDesc difference is handled by metadata. */
    ((FM5UiGetResource)(*(void ***)view)[8])(view, &resource);
    if (!resource) return E_UNEXPECTED;
    status = FM5GetLegacyTexture2DDescription(resource, description);
    ((FM5UiRelease)(*(void ***)resource)[2])(resource);
    if (FAILED(status)) return status;
    if (!description[0] || !description[1] || description[0] > 16384 || description[1] > 16384)
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    *width = description[0]; *height = description[1];
    return S_OK;
}

__declspec(dllexport) void FM5BuildUiViewport(void *ui, float *viewport, UINT logical_height) {
    void *view;
    UINT width = 0, height = 0;
    HRESULT status;
    FM5ObserveColorBeforeUi(ui);
    if (!ui || !viewport || (logical_height != 1080 && logical_height != 540))
        fm5_ui_failure(E_INVALIDARG, ui);
    memcpy(&view, (const BYTE *)ui + 0x618, sizeof(view));
    status = fm5_ui_view_extent(view, &width, &height);
    if (FAILED(status)) fm5_ui_failure(status, view);
    viewport[0] = viewport[1] = viewport[4] = 0.0f;
    viewport[2] = (float)width;
    viewport[3] = (float)height * (logical_height == 540 ? 0.5f : 1.0f);
    viewport[5] = 1.0f;
}

static int fm5_ui_round(float value) {
    if (!(value >= -1048576.0f && value <= 1048576.0f))
        fm5_ui_failure(E_INVALIDARG, NULL);
    return (int)(value + (value > 0.0f ? 0.5f : -0.5f));
}

__declspec(dllexport) void *FM5CaptureUiScaled(void *ui, const float *rectangle) {
    void *context, *view = NULL, *target;
    UINT source_width = 0, source_height = 0;
    int width, height, maximum_width, maximum_height;
    float physical[4], scale_x, scale_y;
    const float *copy_rectangle = rectangle;
    HRESULT status;
    if (!ui || !rectangle || !fm5_ui_allocate_target || !fm5_ui_copy_target)
        fm5_ui_failure(E_POINTER, ui);
    memcpy(&context, (const BYTE *)ui + 0x18, sizeof(context));
    if (!context) fm5_ui_failure(E_POINTER, ui);
    width = fm5_ui_round(rectangle[2]); height = fm5_ui_round(rectangle[3]);
    maximum_width = fm5_ui_round(*(const float *)((const BYTE *)ui + 0x610));
    maximum_height = fm5_ui_round(*(const float *)((const BYTE *)ui + 0x614));
    if (width > maximum_width) width = maximum_width;
    if (height > maximum_height) height = maximum_height;
    /* The separate offscreen Begin path already uses pixel-sized projections.
       Its allocation, coordinates and copy behavior stay exactly original. */
    if (!*((const BYTE *)ui + 0x5f0)) {
        /* Original b5ecb8 is FF 97 F0 02 00 00: the old Context stores
           methods inline, so +0x2f0 is not an offset in its vtable. */
        ((FM5UiGetTargets)(*(void **)((BYTE *)context + 0x2f0)))(context, 1, &view, NULL);
        status = fm5_ui_view_extent(view, &source_width, &source_height);
        if (view) ((FM5UiRelease)(*(void ***)view)[2])(view);
        if (FAILED(status)) fm5_ui_failure(status, context);
        scale_x = (float)source_width / 1920.0f;
        scale_y = (float)source_height / 1080.0f;
        if (source_width != 1920 || source_height != 1080) {
            int left = fm5_ui_round(rectangle[0] * scale_x);
            int top = fm5_ui_round(rectangle[1] * scale_y);
            int right = fm5_ui_round((rectangle[0] + rectangle[2]) * scale_x);
            int bottom = fm5_ui_round((rectangle[1] + rectangle[3]) * scale_y);
            maximum_width = fm5_ui_round((float)maximum_width * scale_x);
            maximum_height = fm5_ui_round((float)maximum_height * scale_y);
            width = right - left; height = bottom - top;
            if (width > maximum_width) width = maximum_width;
            if (height > maximum_height) height = maximum_height;
            physical[0] = (float)left; physical[1] = (float)top;
            physical[2] = (float)(right - left); physical[3] = (float)(bottom - top);
            copy_rectangle = physical;
        }
    }
    if (width <= 0 || height <= 0) fm5_ui_failure(E_INVALIDARG, ui);
    /* Preserve the genuine title's pooled target identity and copy lifetime.
       Only pixel extents change; b61360 keeps its original logical wrapper. */
    target = fm5_ui_allocate_target(ui, (UINT)width, (UINT)height);
    if (!target) fm5_ui_failure(E_OUTOFMEMORY, ui);
    fm5_ui_copy_target(target, context, copy_rectangle);
    return target;
}

/* FM5 DynamicRenderSettings accessors: values follow the original title's
   typed control metadata. Mode1 keeps every original scene/override path. */
typedef struct FM5GraphicsSite {
    uint32_t rva;
    unsigned char group;
    unsigned char kind; /* 0=integer/bool, 1=float, 2=two-int output struct */
    unsigned char original[17];
    uint32_t values[4][2];
} FM5GraphicsSite;

static const FM5GraphicsSite fm5_graphics_sites[] = {
    /* ShadowMapResolution */
    {0x8433c0, 0, 0,
     {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff},
     {{0x00000100u,0x00000000u},{0x00000000u,0x00000000u},{0x00000400u,0x00000000u},{0x00000800u,0x00000000u}}},
    /* MaxHiDefShadows */
    {0x735550, 0, 0,
     {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff},
     {{0x00000004u,0x00000000u},{0x00000000u,0x00000000u},{0x00000010u,0x00000000u},{0x00000010u,0x00000000u}}},
    /* WindshieldReflectionResolution */
    {0x735910, 1, 0,
     {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff},
     {{0x00000100u,0x00000000u},{0x00000000u,0x00000000u},{0x00000400u,0x00000000u},{0x00000800u,0x00000000u}}},
    /* EnvMapFrequencyScale */
    {0x6faad0, 1, 1,
     {0x48,0x8b,0xc4,0x56,0x57,0x41,0x56,0x48,0x83,0xec,0x50,0x48,0xc7,0x40,0xb8,0xfe,0xff},
     {{0x3f000000u,0x00000000u},{0x00000000u,0x00000000u},{0x3f800000u,0x00000000u},{0x40000000u,0x00000000u}}},
    /* HalfRateMirror */
    {0x14e720, 1, 0,
     {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff},
     {{0x00000001u,0x00000000u},{0x00000000u,0x00000000u},{0x00000000u,0x00000000u},{0x00000000u,0x00000000u}}},
    /* CarFocusLODMinMax */
    {0x8aa8b0, 2, 2,
     {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff},
     {{0x00000002u,0x00000005u},{0x00000000u,0x00000000u},{0x00000000u,0x00000005u},{0x00000000u,0x00000005u}}},
    /* CarShaderLODBias */
    {0x6fae00, 2, 0,
     {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff},
     {{0x00000001u,0x00000000u},{0x00000000u,0x00000000u},{0xffffffffu,0x00000000u},{0xfffffffau,0x00000000u}}},
    /* ParticleRateScale */
    {0x859ca0, 3, 1,
     {0x48,0x8b,0xc4,0x56,0x57,0x41,0x56,0x48,0x83,0xec,0x50,0x48,0xc7,0x40,0xb8,0xfe,0xff},
     {{0x3e800000u,0x00000000u},{0x00000000u,0x00000000u},{0x3f800000u,0x00000000u},{0x3f800000u,0x00000000u}}},
    /* ParticleRateScaleForCars */
    {0x8aacf0, 3, 1,
     {0x48,0x8b,0xc4,0x56,0x57,0x41,0x56,0x48,0x83,0xec,0x50,0x48,0xc7,0x40,0xb8,0xfe,0xff},
     {{0x3f000000u,0x00000000u},{0x00000000u,0x00000000u},{0x3f800000u,0x00000000u},{0x3fe00000u,0x00000000u}}},
    /* MotionBlur */
    {0x5c76e0, 4, 0,
     {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff},
     {{0x00000000u,0x00000000u},{0x00000000u,0x00000000u},{0x00000001u,0x00000000u},{0x00000001u,0x00000000u}}},
    /* MotionBlurSamples */
    {0x5c7570, 4, 1,
     {0x48,0x8b,0xc4,0x56,0x57,0x41,0x56,0x48,0x83,0xec,0x50,0x48,0xc7,0x40,0xb8,0xfe,0xff},
     {{0x40a00000u,0x00000000u},{0x00000000u,0x00000000u},{0x41500000u,0x00000000u},{0x41a80000u,0x00000000u}}},
    /* ForceAnisoFiltering */
    {0x8ac6e0, 5, 0,
     {0x40,0x57,0x41,0x56,0x41,0x57,0x48,0x83,0xec,0x40,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff},
     {{0x00000001u,0x00000000u},{0x00000000u,0x00000000u},{0x00000002u,0x00000000u},{0x00000003u,0x00000000u}}},
};

static void fm5_apply_graphics_site(unsigned char *image,
                                    const FM5GraphicsSite *site, uint32_t mode) {
    unsigned char code[17];
    memset(code, 0x90, sizeof(code));
    if (site->kind == 2) {
        code[0] = 0x48; code[1] = 0xb8;
        memcpy(code + 2, site->values[mode], 8);
        memcpy(code + 10, "\x48\x89\x02\x48\x8b\xc2\xc3", 7);
    } else {
        code[0] = 0xb8;
        memcpy(code + 1, site->values[mode], 4);
        if (site->kind == 1)
            memcpy(code + 5, "\xc5\xf9\x6e\xc0\xc3", 5);
        else
            code[5] = 0xc3;
    }
    memcpy(image + site->rva, code, sizeof(code));
}

__declspec(dllexport) HRESULT FM5ValidateNativeProfile(
    const NativeImage *image, const NativeOptions *options) {
    static const unsigned char instruction[] = {0xc5,0xfa,0x10,0x05,0x3c,0xce,0x70,0x01};
    static const unsigned char constants[] = {0x89,0x88,0x88,0x3c,0x86,0xab,0x88,0x3c};
    static const unsigned char height_store[] = {0xc7,0x83,0x60,0x25,0,0,0x38,4,0,0};
    static const unsigned char width_from_height[] = {
        0xc5,0xfa,0x59,0x0d,0x0b,0x2a,0xba,1,
        0xc5,0xf2,0x59,0x15,0x3f,0x22,0xba,1,0xc4,0xe1,0xfa,0x2c,0xea};
    static const unsigned char ui_width[] = {
        0x41,0x8b,0x47,8,0xc5,0xf8,0x57,0xc0,0xc4,0xe1,0xfa,0x2a,0xc0};
    static const unsigned char ui_height[] = {
        0x41,0x8b,0x47,0x0c,0xc5,0xf0,0x57,0xc9,0xc4,0xe1,0xf2,0x2a,0xc8};
    static const unsigned char ui_viewport[] = {
        0xc5,0xfa,0x10,0x05,0x2a,0xd7,0x17,0x01,0xc4,0xc1,0x79,0x6e,0xf0,0xc5,
        0xc0,0x57,0xff,0x4c,0x8d,0x44,0x24,0x30,0xba,0x01,0,0,0,0x48,
        0x8b,0xc8,0xc5,0x7a,0x11,0x44,0x24,0x38,0xc5,0xfa,0x11,0x44,0x24,0x44,
        0xc5,0xf8,0x5b,0xf6,0xc5,0xfa,0x11,0x7c,0x24,0x30,0xc5,0xfa,0x11,0x7c,
        0x24,0x34,0xc5,0xfa,0x11,0x74,0x24,0x3c,0xc5,0xfa,0x11,0x7c,0x24,0x40};
    static const unsigned char ui_capture[] = {
        0xc5,0xfa,0x10,0x87,0x10,0x06,0x00,0x00,0xc5,0xf8,0x2f,0xc2,0x76,0x06,0xc5,0xfa,0x58,0xc1,0xeb,0x04,0xc5,0xfa,0x5c,0xc1,0xc5,0xfa,0x2c,0xd0,0x3b,0xc2,0x0f,0x4c,0xd0,0xc5,0xfa,0x10,0x87,0x14,0x06,0x00,0x00,0xc5,0xf8,0x2f,0xc2,0x76,0x06,0xc5,0xfa,0x58,0xc1,0xeb,0x04,0xc5,0xfa,0x5c,0xc1,0xc5,0x7a,0x2c,0xc0,0x41,0x3b,0xc8,0x44,0x0f,0x4c,0xc1,0x48,0x8b,0xcf,0xe8,0xc2,0x3e,0x00,0x00,0x48,0x8b,0xe8,0x4c,0x8b,0xc6,0x48,0x8b,0x57,0x18,0x48,0x8b,0xc8,0xe8,0x60,0xd8,0xff,0xff};
    static const unsigned char ui_allocate[] = {
        0x40,0x57,0x48,0x83,0xec,0x30,0x48,0xc7,0x44,0x24,0x20,0xfe,0xff,0xff,0xff,0x48,0x89};
    static const unsigned char ui_copy[] = {
        0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x18,0x55,0x57,0x41,0x56,0x48,0x8d,0x6c};
    uint32_t effective_height;
    unsigned int index;
    if (!image || !options || !image->base) return E_POINTER;
    if (image->size != 0x279d000 ||
        memcmp(image->base + 0x5d130a, "\x74\x0f", 2) ||
        memcmp(image->base + 0x5d130c, instruction, sizeof(instruction)) ||
        memcmp(image->base + 0x1cde14c, constants, sizeof(constants)) ||
        memcmp(image->base + 0x5d1ce1, height_store, sizeof(height_store)) ||
        memcmp(image->base + 0x13bfc1, width_from_height, sizeof(width_from_height)) ||
        memcmp(image->base + 0x8e4da5, ui_width, sizeof(ui_width)) ||
        memcmp(image->base + 0x8e4db7, ui_height, sizeof(ui_height)) ||
        memcmp(image->base + 0xb60d72, ui_viewport, sizeof(ui_viewport)) ||
        memcmp(image->base + 0xb60db8, "\xff\x90\x88\x01\0\0", 6) ||
        memcmp(image->base + 0xb613c2, ui_capture, sizeof(ui_capture)) ||
        memcmp(image->base + 0xb652d0, ui_allocate, sizeof(ui_allocate)) ||
        memcmp(image->base + 0xb5ec80, ui_copy, sizeof(ui_copy)))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    for (index = 0; index < sizeof(fm5_graphics_sites) / sizeof(fm5_graphics_sites[0]); ++index) {
        const FM5GraphicsSite *site = &fm5_graphics_sites[index];
        if (memcmp(image->base + site->rva, site->original, sizeof(site->original)))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    }
    effective_height = options->height ? options->height : 1080;
    if ((effective_height != 1080 && effective_height != 1440 &&
         effective_height != 2160 && effective_height != 2880 &&
         effective_height != 3240 && effective_height != 3780 && effective_height != 4320) ||
        (options->width != 0 && options->width != effective_height * 16 / 9) ||
        (options->frame_rate != 30 && options->frame_rate != 60 &&
         !(options->frame_rate == 120 && options->real_time == 1)) ||
        options->real_time > 1 || options->optional_value)
        return E_INVALIDARG;
    for (index = 0; index < 6; ++index)
        if (options->graphics_groups[index] > 3) return E_INVALIDARG;
    return S_OK;
}

__declspec(dllexport) void FM5ApplyPatches(
    const NativeImage *image, const NativeOptions *options) {
    HRESULT result = FM5ValidateNativeProfile(image, options);
    ImportRecord marker = {0};
    marker.library.storage.pointer = "ForzaMastro5";
    marker.library.length = 12;
    marker.library.capacity = 31;
    marker.symbol.storage.pointer = "native-profile";
    marker.symbol.length = 14;
    marker.symbol.capacity = 31;
    if (options) {
        char details[512];
        int count = _snprintf_s(details, sizeof(details), _TRUNCATE,
            "height=%u width=%u fps=%u realtime=%u groups=%u,%u,%u,%u,%u,%u ui=%u optional=%p",
            options->height, options->width, options->frame_rate, (unsigned int)options->real_time,
            options->graphics_groups[0], options->graphics_groups[1], options->graphics_groups[2],
            options->graphics_groups[3], options->graphics_groups[4], options->graphics_groups[5],
            (unsigned int)options->ui_clock, options->optional_value);
        if (count > 0) {
            marker.symbol.storage.pointer = details;
            marker.symbol.length = (size_t)count;
            marker.symbol.capacity = sizeof(details) - 1;
            trace_import(&marker, "native-profile-options", NULL);
        }
        marker.symbol.storage.pointer = "native-profile";
        marker.symbol.length = 14;
        marker.symbol.capacity = 31;
    }
    trace_import(&marker, SUCCEEDED(result) ?
                 (options->frame_rate == 120 ? "native-fm5-120hz-realtime" :
                  options->frame_rate == 30 ? "native-fm5-30hz-profile" : "native-fm5-60hz") :
                 "profile-refused",
                 (FARPROC)(uintptr_t)(uint32_t)result);
    if (FAILED(result)) {
        ULONG_PTR arguments[] = {(ULONG_PTR)(uint32_t)result};
        RaiseException(0xe0f50001, EXCEPTION_NONCONTINUABLE, 1, arguments);
        return;
    }
    {
        wchar_t probe[4];
        if (GetEnvironmentVariableW(L"FM5_NATIVE_APU_PROBE", probe, 4) == 1 && probe[0] == L'1')
            ExitProcess((UINT)FM5RunApuNativeProbe());
    }
    FM5SetSceneFileRate(options->frame_rate);
    if (options->real_time)
        image->base[0x5d130a] = 0xeb;
    else if (options->frame_rate != 30)
        image->base[0x5d1310] = 0x38;
    if (options->height && options->height != 1080) {
        static const unsigned char logical_width[] = {
            0xb8,0x80,7,0,0,0x0f,0x57,0xc0,0xc4,0xe1,0xfa,0x2a,0xc0};
        static const unsigned char logical_height[] = {
            0xb8,0x38,4,0,0,0x0f,0x57,0xc9,0xc4,0xe1,0xf2,0x2a,0xc8};
        unsigned char physical_ui_viewport[] = {
            0xc4,0xc1,0x79,0x6e,0xf0,0xc5,0xf8,0x5b,0xf6,0xc5,0xc0,0x57,0xff,0x48,0x8b,0xcf,0x48,0x8d,0x54,0x24,0x30,0x48,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xd0,0x48,0x8b,0x47,0x18,0x4c,0x8d,0x44,0x24,0x30,0xba,0x01,0x00,0x00,0x00,0x48,0x8b,0xc8,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        unsigned char physical_ui_capture[] = {
            0x48,0x8b,0xcf,0x48,0x8b,0xd6,0x48,0xb8,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xff,0xd0,0x48,0x8b,0xe8,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
        uintptr_t viewport_helper = (uintptr_t)&FM5BuildUiViewport;
        uintptr_t capture_helper = (uintptr_t)&FM5CaptureUiScaled;
        uint32_t height = options->height;
        /* Authored XAML, projection and clip planes stay1920x1080. The main
           viewport follows the actual target; modal captures copy physical pixels
           into genuine title targets while retaining original logical bounds. */
        fm5_ui_allocate_target = (FM5UiAllocateTarget)(image->base + 0xb652d0);
        fm5_ui_copy_target = (FM5UiCopyTarget)(image->base + 0xb5ec80);
        memcpy(physical_ui_viewport + 23, &viewport_helper, sizeof(viewport_helper));
        memcpy(physical_ui_capture + 8, &capture_helper, sizeof(capture_helper));
        memcpy(image->base + 0x5d1ce7, &height, sizeof(height));
        if (options->width) {
            image->base[0x13bfd1] = 0xbd;
            memcpy(image->base + 0x13bfd2, &options->width, sizeof(options->width));
        }
        memcpy(image->base + 0x8e4da5, logical_width, sizeof(logical_width));
        memcpy(image->base + 0x8e4db7, logical_height, sizeof(logical_height));
        memcpy(image->base + 0xb60d72, physical_ui_viewport, sizeof(physical_ui_viewport));
        memcpy(image->base + 0xb613c2, physical_ui_capture, sizeof(physical_ui_capture));
    }
    {
        unsigned int index;
        for (index = 0; index < sizeof(fm5_graphics_sites) / sizeof(fm5_graphics_sites[0]); ++index) {
            const FM5GraphicsSite *site = &fm5_graphics_sites[index];
            uint32_t mode = options->graphics_groups[site->group];
            if (mode != 1) fm5_apply_graphics_site(image->base, site, mode);
        }
    }
}

__declspec(dllexport) FARPROC FM5ResolveImport(const ImportRecord *record) {
    FARPROC target = NULL;
    const char *route = "RaceOneEngine";
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    int valid_host = known_azimuth_host(base);
    if (!record || record->library.length > 128 || record->symbol.length > 512)
        return NULL;
    if (string_is(&record->library, "xg.dll")) {
        HMODULE module = load_original_xg();
        if (module) target = GetProcAddress(module, string_data(&record->symbol));
        route = "original-fm5-xg";
    } else if (string_is(&record->library, "kernelx.dll") &&
               string_is(&record->symbol, "GlobalMemoryStatusEx")) {
        target = (FARPROC)(uintptr_t)&GlobalMemoryStatusEx;
        route = "win32";
    } else if (string_is(&record->library, "d3d11_x.dll") &&
               string_is(&record->symbol, "D3D11CreateDevice")) {
        if (valid_host && FM5ConfigureGraphicsBridge(
            (FM5CreateDeviceX)(base + 0x58090),
            FM5_GRAPHICS_ALLOW_DESCRIPTOR_NORMALIZATION) == 0)
            target = (FARPROC)(uintptr_t)&FM5D3D11CreateDevice;
        route = "fm5-device-adapter";
    } else if (valid_host && string_is(&record->library, "d3d11_x.dll") &&
               string_is(&record->symbol, "DXGIXPresentArray")) {
        target = (FARPROC)(uintptr_t)&FM5DXGIXPresentArray;
        route = "fm5-present-48-to-56";
    } else if (string_is(&record->library, "xaudio2_9.dll") &&
               (string_is(&record->symbol, "#1") || string_is(&record->symbol, "1"))) {
        if (valid_host) target = (FARPROC)(uintptr_t)&FM5XAudio2Create;
        route = "fm5-audio-adapter";
    } else if (valid_host && string_is(&record->library, "acphal.dll") &&
               string_is(&record->symbol, "ApuAlloc")) {
        target = (FARPROC)(uintptr_t)&FM5ApuAlloc;
        route = "fm5-implicit-apu-heap";
    } else if (valid_host && string_is(&record->library, "acphal.dll") &&
               string_is(&record->symbol, "AcpHalCreate")) {
        target = (FARPROC)(uintptr_t)&FM5AcpHalCreate;
        route = "fm5-implicit-apu-heap";
    } else if (valid_host && string_is(&record->library, "acphal.dll") &&
               string_is(&record->symbol, "AcpHalAllocateShapeContexts")) {
        target = (FARPROC)(uintptr_t)&FM5AcpHalAllocateShapeContexts;
        route = "fm5-implicit-apu-heap";
    } else if (valid_host && string_is(&record->library, "vccorlib110.dll") &&
               string_is(&record->symbol, "?GetActivationFactoryByPCWSTR@@YAJPEAXAEAVGuid@Platform@@PEAPEAX@Z")) {
        FARPROC original = ((ResolveImport)(base + 0x3a290))(record);
        if (SUCCEEDED(FM5ConfigureActivation((FM5ActivationBackend)(uintptr_t)original)))
            target = (FARPROC)(uintptr_t)&FM5GetActivationFactory;
        route = "fm5-winrt-activation";
    } else if (valid_host) {
        target = ((ResolveImport)(base + 0x3a290))(record);
    }
    trace_import(record, route, target);
    return target;
}
