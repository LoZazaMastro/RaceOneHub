#ifndef FM5_FRAME_TIMING_H
#define FM5_FRAME_TIMING_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

__declspec(dllexport) DWORD WINAPI FM5FrameLatencyWait(HANDLE object, DWORD milliseconds);
__declspec(dllexport) DWORD WINAPI FM5PacingTimerWait(HANDLE object, DWORD milliseconds);
__declspec(dllexport) LONGLONG WINAPI FM5PacingDeadline(void *state, LONGLONG now,
    LONGLONG period, LONGLONG display_period, LONGLONG phase);

#ifdef FM5_FRAME_TIMING_TEST
typedef DWORD (WINAPI *FM5TimingWaitBackend)(HANDLE, DWORD);
typedef LONGLONG (WINAPI *FM5TimingDeadlineBackend)(void *, LONGLONG,
    LONGLONG, LONGLONG, LONGLONG);
void FM5FrameTimingTestConfigure(FM5TimingWaitBackend wait_backend,
    FM5TimingDeadlineBackend deadline_backend, BOOL enabled);
HRESULT FM5FrameTimingTestGuard(const BYTE *base, SIZE_T size);
#endif
#endif
