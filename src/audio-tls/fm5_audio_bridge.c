#include "fm5_audio_bridge.h"
#include <string.h>

#ifdef FM5_BRIDGE_TEST
static FM5_XAUDIO_CREATE test_backend;
void FM5AudioSetTestBackend(FM5_XAUDIO_CREATE backend) { test_backend = backend; }
#endif

static FM5_XAUDIO_CREATE backend(void) {
#ifdef FM5_BRIDGE_TEST
    return test_backend;
#else
    static const unsigned char entry[] = {
        0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x50,
        0x48,0x8b,0x05,0x1f,0x67,0x19,0x00
    };
    unsigned char *host = (unsigned char *)GetModuleHandleW(NULL);
    if (!host) return NULL;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)host;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0x40 || dos->e_lfanew > 0x1000) return NULL;
    IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(host + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.SizeOfImage < 0x181b10 + sizeof(entry) ||
        memcmp(host + 0x181b10, entry, sizeof(entry))) return NULL;
    return (FM5_XAUDIO_CREATE)(host + 0x181b10);
#endif
}

HRESULT WINAPI FM5XAudio2Create(void **output, UINT32 flags, UINT32 processor, void *context) {
    if (!output) return E_POINTER;
    *output = NULL;
    if (flags != 0 || processor != 0x400 || context != NULL) return E_INVALIDARG;
    FM5_XAUDIO_CREATE create = backend();
    if (!create) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    return create(output, flags, 0xffffffffu, context);
}
