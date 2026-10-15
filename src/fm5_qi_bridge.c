#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

typedef HRESULT (*NativeQuery)(void *, const GUID *, void **);
typedef ULONG (*NativeRelease)(void *);

HRESULT FM5WrapPerformanceContext(void *native_owned, void **output);
HRESULT FM5WrapPerformanceDevice(void *native_owned, void **output);

static const GUID performance_context = {
    0x9458fe06, 0xc78d, 0x47f7, {0x96,0xa0,0xec,0x7b,0x72,0x7b,0xe1,0xe9}
};
static const GUID performance_device = {
    0x88671610, 0x712e, 0x4f1e, {0x84,0xab,0x01,0xb5,0x94,0x8b,0xd3,0x73}
};

#ifdef FM5_QI_BRIDGE_TEST
static NativeQuery test_backend;
void FM5SetQueryTestBackend(NativeQuery backend) { test_backend = backend; }
#endif

static NativeQuery query_backend(void) {
#ifdef FM5_QI_BRIDGE_TEST
    return test_backend;
#else
    static const unsigned char expected[] = {
        0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x56,0x57,
        0x41,0x56,0x48,0x83,0xec,0x20,0x4d,0x8b,0xf0,0x48,0x8b,0xda
    };
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS64 *nt;
    if (!base) return NULL;
    dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0x40 || dos->e_lfanew > 0x1000)
        return NULL;
    nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.SizeOfImage < 0xdb760 + sizeof(expected) ||
        memcmp(base + 0xdb760, expected, sizeof(expected))) return NULL;
    return (NativeQuery)(base + 0xdb760);
#endif
}

__declspec(dllexport) HRESULT FM5GraphicsQueryInterface(void *object, const GUID *iid, void **output) {
    NativeQuery original;
    HRESULT result;
    void *native, *wrapped = NULL;
    if (!output) return E_POINTER;
    *output = NULL;
    if (!object || !iid) return E_POINTER;
    original = query_backend();
    if (!original) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    result = original(object, iid, output);
    if (FAILED(result)) return result;
    if (!*output) return E_UNEXPECTED;
    native = *output;
    if (!memcmp(iid, &performance_context, sizeof(GUID)))
        result = FM5WrapPerformanceContext(native, &wrapped);
    else if (!memcmp(iid, &performance_device, sizeof(GUID)))
        result = FM5WrapPerformanceDevice(native, &wrapped);
    else
        return result;
    if (FAILED(result) || !wrapped) {
        void **vtable = *(void ***)native;
        ((NativeRelease)vtable[2])(native);
        *output = NULL;
        return FAILED(result) ? result : E_UNEXPECTED;
    }
    *output = wrapped;
    return result;
}
