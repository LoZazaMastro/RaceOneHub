#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string.h>

void FM5ForgetLegacyResource(void *resource);
void FM5ForgetPrivateData(void *resource);
typedef void (*DestroyResource)(void *);

void FM5RunResourceDestruction(void *resource, DestroyResource original) {
    FM5ForgetPrivateData(resource);
    FM5ForgetLegacyResource(resource);
    original(resource);
}

__declspec(dllexport) void FM5DestroyLegacyResource(void *resource) {
    static const unsigned char signature[] = {
        0x48,0x89,0x5c,0x24,0x18,0x55,0x56,0x57,0x41,0x56,0x41,0x57,
        0x48,0x8b,0xec,0x48,0x83,0xec,0x60,0x48,0x8b,0xf9,0x48,0x8b
    };
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS64 *nt;
    if (!base) goto failure;
    dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0x40 || dos->e_lfanew > 0x1000) goto failure;
    nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.SizeOfImage < 0xda270 + sizeof(signature) ||
        memcmp(base + 0xda270, signature, sizeof(signature))) goto failure;
    FM5RunResourceDestruction(resource, (DestroyResource)(base + 0xda270));
    return;
failure:
    RaiseException(0xe0f50009, EXCEPTION_NONCONTINUABLE, 0, NULL);
}
