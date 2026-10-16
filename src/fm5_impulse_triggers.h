#ifndef FM5_IMPULSE_TRIGGERS_H
#define FM5_IMPULSE_TRIGGERS_H
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
/* The host's own gamepad state, read where its native setter reads it. */
typedef struct FM5TriggerHost {
    volatile DWORD *connected;  /* bit per guest pad index */
    volatile DWORD *applied;    /* XInput motor words per pad: low left, high right */
    HWND volatile *owner;
    volatile BYTE *bypass;
    HWND (WINAPI *foreground)(void);
} FM5TriggerHost;
/* Controller input in XInput's layout, used to find a guest pad's device. */
typedef struct FM5TriggerInput {WORD buttons;BYTE left,right;} FM5TriggerInput;
typedef struct FM5TriggerDevice {
    HRESULT (*open)(void);
    UINT (*pads)(void **ids,FM5TriggerInput *inputs,UINT capacity);
    HRESULT (*vibrate)(void *id,const float motors[4]);
    BOOL (*xinput)(DWORD index,FM5TriggerInput *input);
} FM5TriggerDevice;
BOOL FM5ImpulseTriggersEnabled(void);
/* Called after the native setter accepted a request: motors, then left and right trigger. */
void FM5ImpulseTriggersRequest(const FM5TriggerHost *host,DWORD index,const float request[4]);
const FM5TriggerDevice *FM5ImpulseTriggerDevice(void);
#ifdef FM5_IMPULSE_TRIGGERS_TEST
void FM5ImpulseTriggersSetTest(const FM5TriggerDevice *device,BOOL enabled,BOOL logging);
BOOL FM5ImpulseTriggersStep(ULONGLONG now);
BOOL FM5ImpulseTriggersWanted(DWORD index,float *left,float *right);
#endif
#ifdef __cplusplus
}
#endif
#endif
