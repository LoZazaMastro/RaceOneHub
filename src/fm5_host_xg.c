#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include <wchar.h>

typedef HRESULT (WINAPI *TextureFactory)(const void *, void **);
typedef ULONG (WINAPI *TextureRelease)(void *);
static HMODULE modern_module;
static SRWLOCK modern_lock = SRWLOCK_INIT;

static HMODULE load_modern_xg(void) {
    wchar_t path[32768];
    HMODULE self = NULL;
    HMODULE result;
    AcquireSRWLockExclusive(&modern_lock);
    if (!modern_module && GetModuleHandleExW(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
        (LPCWSTR)(uintptr_t)&load_modern_xg, &self)) {
        DWORD length = GetModuleFileNameW(self, path, 32768);
        wchar_t *slash = length && length < 32768 ? wcsrchr(path, L'\\') : NULL;
        if (slash) {
            slash[1] = 0;
#ifdef FM5_HOST_XG_TEST
            if (!wcscat_s(path, 32768, L"..\\..\\runtime\\xg-host\\fm5-host-xg.dll"))
                modern_module = LoadLibraryExW(path, NULL,
                    LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
#else
            SIZE_T directory_length = wcslen(path);
            static const wchar_t *relative[] = {
                L"xg-host\\fm5-host-xg.dll", L"..\\xg-host\\fm5-host-xg.dll"};
            for (UINT index = 0; index < ARRAYSIZE(relative) && !modern_module; ++index) {
                path[directory_length] = 0;
                if (!wcscat_s(path, 32768, relative[index]))
                    modern_module = LoadLibraryExW(path, NULL,
                        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
            }
#endif
        }
    }
    result = modern_module;
    ReleaseSRWLockExclusive(&modern_lock);
    return result;
}

static const BYTE *xg_module_base(void *address) {
    MEMORY_BASIC_INFORMATION memory;
    const BYTE *base;
    const IMAGE_DOS_HEADER *dos;
    const IMAGE_NT_HEADERS64 *pe;
    if (!address || !VirtualQuery(address, &memory, sizeof(memory)) ||
        memory.State != MEM_COMMIT || (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return 0;
    base = (const BYTE *)memory.AllocationBase;
    if (!VirtualQuery(base, &memory, sizeof(memory)) ||
        memory.State != MEM_COMMIT || memory.RegionSize < 0x1000 ||
        (memory.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return 0;
    dos = (const IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0x40 || dos->e_lfanew > 0x800) return 0;
    pe = (const IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        pe->OptionalHeader.SizeOfImage != 0x1b000 ||
        !((pe->OptionalHeader.AddressOfEntryPoint == 0xc6a8 && pe->FileHeader.TimeDateStamp == 0x52820d2e) ||
          (pe->OptionalHeader.AddressOfEntryPoint == 0xc738 && pe->FileHeader.TimeDateStamp == 0x52820d27))) return NULL;
    return base;
}

static UINT factory_dimension(void *address) {
    static const DWORD offsets[] = {0x2a70, 0x2b60, 0x2c50};
    static const BYTE signature[] = {
        0x48,0x89,0x6c,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,
        0xfa,0x48,0x8b,0xe9,0x48,0x85,0xc9,0x0f,0x84,0xbb,0x00,0x00,
        0x00,0x48,0x85,0xd2,0x0f,0x84,0xb2,0x00
    };
    const BYTE *base = xg_module_base(address);
    UINT dimension = 0;
    if (!base) return 0;
    for (UINT i = 0; i < 3; ++i)
        if ((uintptr_t)address == (uintptr_t)base + offsets[i]) dimension = i + 1;
    if (!dimension || memcmp(address, signature, sizeof(signature))) return 0;
    return dimension;
}

__declspec(dllexport) HRESULT WINAPI FM5CreateHostTextureComputer(
    const void *new_description, void **out, void *original_factory) {
    static const DWORD factory_rvas[] = {0, 0x33e0, 0x34d0, 0x35c0};
    static const BYTE factory_guard[] = {
        0x48,0x89,0x6c,0x24,0x10,0x57,0x48,0x83,
        0xec,0x20,0x48,0x8b,0xfa,0x48,0x8b,0xe9
    };
    const BYTE *base;
    const IMAGE_NT_HEADERS64 *pe;
    HMODULE module;
    FARPROC factory;
    void *original = NULL;
    UINT dimension;
    HRESULT hr;
    if (!out) return E_POINTER;
    *out = NULL;
    dimension = factory_dimension(original_factory);
    if (!dimension) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    if (!new_description) return E_POINTER;
    module = load_modern_xg();
    if (!module) return HRESULT_FROM_WIN32(GetLastError() ? GetLastError() : ERROR_MOD_NOT_FOUND);
    base = (const BYTE *)module;
    pe = (const IMAGE_NT_HEADERS64 *)(base + ((const IMAGE_DOS_HEADER *)base)->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->OptionalHeader.SizeOfImage != 0x1d000 ||
        pe->FileHeader.TimeDateStamp != 0x599f6907 || pe->OptionalHeader.AddressOfEntryPoint != 0xdf84)
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    factory = GetProcAddress(module, dimension == 1 ? "XGCreateTexture1DComputer" :
        dimension == 2 ? "XGCreateTexture2DComputer" : "XGCreateTexture3DComputer");
    if ((const BYTE *)(uintptr_t)factory != base + factory_rvas[dimension] ||
        memcmp((const void *)(uintptr_t)factory, factory_guard, sizeof(factory_guard)))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    hr = ((TextureFactory)(uintptr_t)factory)(new_description, &original);
    if (SUCCEEDED(hr) && original) *out = original;
    else if (SUCCEEDED(hr)) hr = E_UNEXPECTED;
    if (FAILED(hr) && original)
        ((TextureRelease)(*(void ***)original)[1])(original);
    return hr;
}
