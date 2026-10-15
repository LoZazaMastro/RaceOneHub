#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

typedef void (*CompletionSetter)(void *, BOOL);

int FM5VerifyCompletionSetter(const unsigned char *base, size_t size, const void *setter) {
    static const unsigned char immediate[] = {
        0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,
        0x48,0x83,0xec,0x60,0x8b,0xea,0x48,0x8b,0xd9,0x80,0xb9,0x48,0x0c,0x00,0x00,0x00
    };
    static const unsigned char recording[] = {
        0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x40,0x8b,0xda,0x48,0x8b,0xf9,0x48,
        0x8b,0xd1,0x48,0x8d,0x4c,0x24,0x30,0xe8,0xa4,0x45,0xfa,0xff,0xc7,0x44,0x24,0x20
    };
    if (!base) return 0;
    if (setter == base + 0x12cb90)
        return size >= 0x12cb90 + sizeof(immediate) &&
            !memcmp(base + 0x12cb90, immediate, sizeof(immediate));
    if (setter == base + 0x11e940)
        return size >= 0x11e940 + sizeof(recording) &&
            !memcmp(base + 0x11e940, recording, sizeof(recording));
    return 0;
}

#ifdef FM5_COMPLETION_TEST
static unsigned char *test_base;
static size_t test_size;
static CompletionSetter test_setter;
void FM5SetCompletionTestBackend(unsigned char *base, size_t size, CompletionSetter setter) {
    test_base = base;
    test_size = size;
    test_setter = setter;
}
#endif

HRESULT FM5ApplyNativeCompletionPolicy(void *native_context, BOOL enabled) {
    unsigned char *base;
    size_t size;
    void **table;
    CompletionSetter setter;
    if (!native_context) return E_POINTER;
    table = *(void ***)native_context;
    if (!table) return E_POINTER;
    setter = (CompletionSetter)table[0xc4];
#ifdef FM5_COMPLETION_TEST
    base = test_base;
    size = test_size;
    if (!FM5VerifyCompletionSetter(base, size, base ? base + 0x12cb90 : NULL) ||
        setter != test_setter) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
#else
    {
        IMAGE_DOS_HEADER *dos;
        IMAGE_NT_HEADERS64 *nt;
        base = (unsigned char *)GetModuleHandleW(NULL);
        if (!base) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        dos = (IMAGE_DOS_HEADER *)base;
        if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0x40 || dos->e_lfanew > 0x1000)
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
        if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
            nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        size = nt->OptionalHeader.SizeOfImage;
        if (!FM5VerifyCompletionSetter(base, size, (const void *)setter))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    }
#endif
    setter(native_context, enabled != FALSE);
    return S_OK;
}
