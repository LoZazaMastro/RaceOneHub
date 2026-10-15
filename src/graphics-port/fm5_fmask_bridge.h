#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
typedef struct FM5FMaskPlan {
    UINT width, height, format, element_bytes, fragments, coverage_samples;
    UINT expanded_value;
} FM5FMaskPlan;
/* S_FALSE: ordinary view. S_OK: supported expanded FMASK. Failure: a
 * recognized metadata view requires an unimplemented representation. */
HRESULT FM5SelectExpandedFMask(const UINT texture2d[12], const UINT srv[6], FM5FMaskPlan *plan);
HRESULT WINAPI FM5CreateLegacyShaderResourceView(void *native_device,
    void *native_resource, const void *description, void **out);
#ifdef FM5_FMASK_TEST
void FM5FMaskFailAllocation(LONG countdown);
#endif
