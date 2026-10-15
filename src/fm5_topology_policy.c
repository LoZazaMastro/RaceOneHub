#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>

HRESULT FM5TopologyPacket(UINT topology, uint64_t *packet) {
    static const uint64_t packets[] = {
        0, UINT64_C(0x3ffc0009e00), UINT64_C(0x5ffc0009e00), UINT64_C(0x7ffc0009e00),
        UINT64_C(0x9ffc0009e00), UINT64_C(0xdffc0009e00)
    };
    if (!packet) return E_POINTER;
    *packet = 0;
    if (topology < 1 || topology > 5) return E_NOTIMPL;
    *packet = packets[topology];
    return S_OK;
}

static int supported_native_context(void *native_context) {
#ifdef FM5_TOPOLOGY_TEST
    return native_context != NULL;
#else
    static const unsigned char decoder_signature[] = {
        0x48,0xb8,0x00,0x9e,0x00,0xc0,0xff,0x03,0x00,0x00,0x48,0x3b,
        0xc8,0x74,0x59,0x48,0xb8,0x00,0x9e,0x00,0xc0,0xff,0x05,0x00
    };
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS64 *nt;
    void **table;
    if (!base || !native_context) return 0;
    dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0x40 || dos->e_lfanew > 0x1000) return 0;
    nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.SizeOfImage < 0x1260b0 + sizeof(decoder_signature) ||
        memcmp(base + 0x1260b0, decoder_signature, sizeof(decoder_signature))) return 0;
    table = *(void ***)native_context;
    return table && table[3] == base + 0xdbe90;
#endif
}

HRESULT FM5ApplyNativeTopology(void *native_context, UINT topology) {
    uint64_t packet;
    HRESULT status = FM5TopologyPacket(topology, &packet);
    unsigned char *bytes = (unsigned char *)native_context;
    if (FAILED(status)) return status;
    if (!native_context) return E_POINTER;
    if (!supported_native_context(native_context)) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    /* These are the fast-context fields written by the newer SDK's inline API. */
    memcpy(bytes + 0x170, &packet, sizeof(packet));
    *(uint32_t *)(bytes + 0x168) |= UINT32_C(0x46);
    return S_OK;
}
