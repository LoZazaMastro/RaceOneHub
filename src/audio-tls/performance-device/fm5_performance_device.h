#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stdint.h>

#define FM5_E_UNMAPPED_DEVICE_SLOT ((DWORD)0xe05f0054)
typedef struct FM5ScratchDescription { UINT32 flags; UINT32 bytes; } FM5ScratchDescription;

HRESULT WINAPI FM5WrapPerformanceDevice(void *native_owned, void **out);
void *WINAPI FM5UnwrapPerformanceDevice(void *candidate);
BOOL WINAPI FM5InspectScratch(void *candidate, FM5ScratchDescription *out);
#ifdef FM5_PERFORMANCE_TEST
void FM5PerformanceFailAllocation(LONG countdown);
#endif
