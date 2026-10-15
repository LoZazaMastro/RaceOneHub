#include "fm5_presentation.h"
#include <stdint.h>
#include <string.h>

typedef char FM5CheckOldCompositionSize[sizeof(FM5LegacyPresentParameters) == 48 ? 1 : -1];
typedef char FM5CheckOldCookieOffset[offsetof(FM5LegacyPresentParameters, cookie) == 40 ? 1 : -1];
typedef char FM5CheckNewCompositionSize[sizeof(FM5NativePresentParameters) == 56 ? 1 : -1];
typedef char FM5CheckNewFlagsOffset[offsetof(FM5NativePresentParameters, flags) == 48 ? 1 : -1];

static const BYTE present_signature[32] = {
    0x40,0x53,0x56,0x57,0x48,0x81,0xec,0x30,0x01,0x00,0x00,0x48,0x8b,0x05,0x2e,0x73,
    0x2c,0x00,0x48,0x33,0xc4,0x48,0x89,0x84,0x24,0x20,0x01,0x00,0x00,0x41,0x8b,0xd8
};
static FM5PresentArrayBackend native_present;

static HRESULT validate_backend(const BYTE *base, SIZE_T size) {
    if (!base) return E_POINTER;
    if (size < 0x50f00 + sizeof(present_signature) ||
        memcmp(base + 0x50f00, present_signature, sizeof(present_signature)))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    return S_OK;
}

#ifndef FM5_PRESENTATION_TEST
static INIT_ONCE present_once = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK initialize_backend(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    const BYTE *base = (const BYTE *)GetModuleHandleW(NULL);
    const IMAGE_NT_HEADERS64 *pe;
    (void)once; (void)parameter; (void)context;
    if (!base) return FALSE;
    pe = (const IMAGE_NT_HEADERS64 *)(base + ((const IMAGE_DOS_HEADER *)base)->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE ||
        pe->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        FAILED(validate_backend(base, pe->OptionalHeader.SizeOfImage))) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return FALSE;
    }
    native_present = (FM5PresentArrayBackend)(base + 0x50f00);
    return TRUE;
}
#else
static BOOL fail_allocation;
static LONG allocation_balance;
void FM5PresentationTestBackend(FM5PresentArrayBackend backend) { native_present = backend; }
void FM5PresentationTestFailAllocation(BOOL fail) { fail_allocation = fail; }
LONG FM5PresentationTestAllocationBalance(void) { return allocation_balance; }
HRESULT FM5PresentationTestGuard(const BYTE *base, SIZE_T size) {
    return validate_backend(base, size);
}
#endif

__declspec(dllexport) HRESULT WINAPI FM5DXGIXPresentArray(UINT sync_interval,
    UINT threshold, UINT flags, UINT count, void *const *swapchains,
    const FM5LegacyPresentParameters *parameters) {
    FM5NativePresentParameters local;
    FM5NativePresentParameters *converted = NULL;
    HRESULT hr;
#ifndef FM5_PRESENTATION_TEST
    if (!InitOnceExecuteOnce(&present_once, initialize_backend, NULL, NULL))
        return HRESULT_FROM_WIN32(GetLastError());
#else
    if (!native_present) return E_UNEXPECTED;
#endif
    if (!parameters)
        return native_present(sync_interval, threshold, flags, count, swapchains, NULL);
    if (count <= 1) {
        memset(&local, 0, sizeof(local));
        if (count) memcpy(&local.legacy, parameters, sizeof(local.legacy));
        converted = &local;
    } else {
        SIZE_T bytes;
        if ((SIZE_T)count > SIZE_MAX / sizeof(*converted))
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        bytes = (SIZE_T)count * sizeof(*converted);
#ifdef FM5_PRESENTATION_TEST
        if (fail_allocation) return E_OUTOFMEMORY;
#endif
        converted = (FM5NativePresentParameters *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, bytes);
        if (!converted) return E_OUTOFMEMORY;
#ifdef FM5_PRESENTATION_TEST
        InterlockedIncrement(&allocation_balance);
#endif
        for (UINT i = 0; i < count; ++i)
            memcpy(&converted[i].legacy, &parameters[i], sizeof(converted[i].legacy));
    }
    hr = native_present(sync_interval, threshold, flags, count, swapchains, converted);
    if (converted != &local) {
        HeapFree(GetProcessHeap(), 0, converted);
#ifdef FM5_PRESENTATION_TEST
        InterlockedDecrement(&allocation_balance);
#endif
    }
    return hr;
}
