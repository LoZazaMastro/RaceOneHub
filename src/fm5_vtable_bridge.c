#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "audio-tls/private-data/fm5_private_data.h"
#include "graphics-port/fm5_dxgi_bridge.h"

void FM5LegacyGetDescription(void *resource, void *description);
typedef void **(*BuildVtable)(void **, UINT);

static UINT getter_rva(UINT kind) {
    switch (kind) {
    case 5: return 0xeab70;
    case 0xc: return 0xeab90;
    case 0xd: return 0xeabd0;
    case 0x10: return 0xeac00;
    default: return 0;
    }
}

HRESULT FM5PatchResourceVtable(void **table, UINT kind, const unsigned char *base, size_t size) {
    static const UINT private_slots[] = {0xf6330, 0xf6430, 0xf6530, 0xf6630};
    UINT getter = getter_rva(kind);
    UINT index;
    if (!table || !base) return E_POINTER;
    if (kind == 0xe || kind == 0xf) return FM5PatchFactoryVtable(table, kind, base, size);
    if (kind != 8 && !getter) return E_INVALIDARG;
    if (size < 0xf6630 + 16) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    for (index = 0; index < 4; ++index)
        if (table[index + 4] != base + private_slots[index])
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    if (getter && (size < 0xf6a30 + 16 || table[11] != base + 0xf6a30 || table[12] != base + getter))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    table[4] = (void *)FM5GetPrivateData;
    table[5] = (void *)FM5SetPrivateData;
    table[6] = (void *)FM5SetPrivateDataInterface;
    table[7] = (void *)FM5SetPrivateDataInterface2;
    if (getter) table[11] = (void *)FM5LegacyGetDescription;
    return S_OK;
}

__declspec(dllexport) void **FM5InitializeResourceVtable(void **table, UINT kind) {
    static const unsigned char signature[] = {
        0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x81,0xec,0x90,0x08,0x00,
        0x00,0x48,0x63,0xfa,0x48,0x8b,0xd9,0x33,0xd2,0x48,0x8d,0x4c
    };
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS64 *nt;
    HRESULT status = HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    void **result;
    if (!table || (kind != 8 && kind != 0xe && kind != 0xf && !getter_rva(kind)) || !base) goto failure;
    dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0x40 || dos->e_lfanew > 0x1000) goto failure;
    nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.SizeOfImage < 0xf36d0 + sizeof(signature) ||
        memcmp(base + 0xf36d0, signature, sizeof(signature))) goto failure;
    result = ((BuildVtable)(base + 0xf36d0))(table, kind);
    if (result != table) goto failure;
    status = FM5PatchResourceVtable(table, kind, base, nt->OptionalHeader.SizeOfImage);
    if (FAILED(status)) goto failure;
    return result;
failure:
    {
        ULONG_PTR details[] = {kind, (ULONG_PTR)(uint32_t)status};
        RaiseException(0xe0f50008, EXCEPTION_NONCONTINUABLE, 2, details);
    }
    return NULL;
}
