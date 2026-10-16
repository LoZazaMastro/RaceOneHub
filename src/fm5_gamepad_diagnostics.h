#ifndef FM5_GAMEPAD_DIAGNOSTICS_H
#define FM5_GAMEPAD_DIAGNOSTICS_H
#include <windows.h>
__declspec(dllexport) HRESULT WINAPI FM5SetGamepadVibration(void *self,const float *vibration);
BOOL FM5InstallGamepadVibrationHook(void);
#ifdef FM5_GAMEPAD_DIAGNOSTICS_TEST
typedef struct FM5GamepadTestBackend {
    HRESULT (WINAPI *set)(void *,const float *);
    HWND (WINAPI *foreground)(void);
    void **vtable;
    volatile DWORD *mask634,*mask638,*cache;
    HWND volatile *owner;
    volatile BYTE *bypass;
} FM5GamepadTestBackend;
void FM5GamepadSetTestBackend(const FM5GamepadTestBackend *backend);
void FM5GamepadSetTestTime(ULONGLONG ticks);
#endif
#endif
