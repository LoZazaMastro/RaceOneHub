#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stddef.h>

/* FM5 UMDPresentArray traverses these records at stride 0x30. */
typedef struct FM5LegacyPresentParameters {
    BOOL disable;
    BOOL use_previous_buffer;
    RECT source_rect;
    POINT destination_origin;
    FLOAT scale_vertical;
    FLOAT scale_horizontal;
    void *cookie;
} FM5LegacyPresentParameters;

typedef struct FM5NativePresentParameters {
    FM5LegacyPresentParameters legacy;
    UINT flags;
} FM5NativePresentParameters;

typedef HRESULT (WINAPI *FM5PresentArrayBackend)(UINT, UINT, UINT, UINT,
    void *const *, const FM5NativePresentParameters *);

__declspec(dllexport) HRESULT WINAPI FM5DXGIXPresentArray(UINT sync_interval,
    UINT threshold, UINT flags, UINT count, void *const *swapchains,
    const FM5LegacyPresentParameters *parameters);

#ifdef FM5_PRESENTATION_TEST
void FM5PresentationTestBackend(FM5PresentArrayBackend backend);
void FM5PresentationTestFailAllocation(BOOL fail);
LONG FM5PresentationTestAllocationBalance(void);
HRESULT FM5PresentationTestGuard(const BYTE *base, SIZE_T size);
#endif
