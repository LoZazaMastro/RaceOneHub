#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
HRESULT WINAPI FM5ReadNativeScissorSnapshot(void *native_context,UINT *count,void *rectangles256);
HRESULT WINAPI FM5GetNativePixelSamplers(void *native_context,UINT start,UINT count,void **samplers);
typedef struct FM5ScissorLockRecord {
    PSRWLOCK mutex;
    BYTE held;
    BYTE reserved[7];
} FM5ScissorLockRecord;
#ifdef FM5_SCISSOR_STATE_TEST
typedef void (*FM5ScissorWaitForTest)(void *native_device,UINT64 published_sequence);
typedef FM5ScissorLockRecord *(*FM5ScissorLockForTest)(FM5ScissorLockRecord *record,void *context);
typedef ULONG (*FM5SamplerAddRefForTest)(void *sampler);
HRESULT FM5TestReadNativeScissorSnapshot(void *native_context,UINT *count,void *rectangles256,
                                      const BYTE *host_base,FM5ScissorWaitForTest wait);
HRESULT FM5TestReadNativeScissorSnapshotLocked(void *native_context,UINT *count,void *rectangles256,
    const BYTE *host_base,FM5ScissorWaitForTest wait,FM5ScissorLockForTest acquire);
HRESULT FM5TestGetNativePixelSamplers(void *native_context,UINT start,UINT count,void **samplers,
    const BYTE *host_base,FM5ScissorWaitForTest wait,FM5ScissorLockForTest acquire,FM5SamplerAddRefForTest add_ref);
#endif
