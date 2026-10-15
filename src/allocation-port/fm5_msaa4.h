#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include "../graphics-port/fm5_fmask_bridge.h"
#ifdef __cplusplus
extern "C" {
#endif
typedef void (*FM5PixelShaderObserver)(void *native_shader,const void *original_bytecode,SIZE_T original_bytes);
/* Configured before guest startup; observer borrows all arguments synchronously. */
void FM5ConfigurePixelShaderObserver(FM5PixelShaderObserver observer);
BOOL FM5ForcedFourSampleMode(void);
UINT FM5ForcedSampleCount(void);
__declspec(dllexport) UINT WINAPI FM5ParseNativeAntialiasing(const void *wstring,UINT fallback);
__declspec(dllexport) UINT WINAPI FM5MapNativeSampleMask(UINT effective_count,
    UINT original_count,UINT original_quality,UINT original_mask);
HRESULT FM5SelectFourSampleResolve(const void *original, SIZE_T bytes, UINT requested_samples,
                                  const void **replacement, SIZE_T *replacement_bytes);
HRESULT FM5SelectFourSampleFMask(const UINT texture[12], const UINT srv[6], FM5FMaskPlan *plan);
HRESULT WINAPI FM5CreateLegacyPixelShader(void *native_device, const void *bytecode,
    SIZE_T bytes, void *class_linkage, void **out);
HRESULT WINAPI FM5ProbeFourSampleDevice(void *native_device, UINT *physical_samples,
    UINT *physical_quality, UINT *created_pixel_shaders);
#ifdef FM5_MSAA4_TEST
void FM5SetMsaaTestHost(const BYTE *host);
#endif
#ifdef __cplusplus
}
#endif
