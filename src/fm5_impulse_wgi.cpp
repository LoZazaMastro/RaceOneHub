#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <roapi.h>
#include <winstring.h>
#include <xinput.h>
#include <windows.gaming.input.h>
#include "fm5_impulse_triggers.h"

/* Windows.Gaming.Input is the Windows interface that drives the trigger motors
   of Xbox One and Series controllers, over USB, the wireless adapter and
   Bluetooth. Everything here runs on the impulse trigger worker thread. combase
   and XInput are loaded at run time so the Bridge's own imports stay unchanged. */
using namespace ABI::Windows::Gaming::Input;
typedef HRESULT (WINAPI *RoInitializeProc)(RO_INIT_TYPE);
typedef HRESULT (WINAPI *RoGetActivationFactoryProc)(HSTRING,REFIID,void **);
typedef HRESULT (WINAPI *WindowsCreateStringReferenceProc)(PCWSTR,UINT32,HSTRING_HEADER *,HSTRING *);
typedef DWORD (WINAPI *XInputGetStateProc)(DWORD,XINPUT_STATE *);
enum {LISTED=8};
static IGamepadStatics *statics;
static IGamepad *listed[LISTED];
static void *identities[LISTED];
static UINT listed_count;
static XInputGetStateProc xinput_get_state;

static HRESULT open_device(void) {
    static const WCHAR name[]=L"Windows.Gaming.Input.Gamepad";
    HMODULE combase=LoadLibraryExW(L"combase.dll",NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
    HMODULE xinput=LoadLibraryExW(L"xinput1_4.dll",NULL,LOAD_LIBRARY_SEARCH_SYSTEM32);
    if(xinput)xinput_get_state=(XInputGetStateProc)(void *)GetProcAddress(xinput,"XInputGetState");
    if(!combase)return HRESULT_FROM_WIN32(GetLastError());
    RoInitializeProc initialize=(RoInitializeProc)(void *)GetProcAddress(combase,"RoInitialize");
    RoGetActivationFactoryProc factory=(RoGetActivationFactoryProc)(void *)GetProcAddress(combase,"RoGetActivationFactory");
    WindowsCreateStringReferenceProc reference=(WindowsCreateStringReferenceProc)(void *)GetProcAddress(combase,"WindowsCreateStringReference");
    if(!initialize || !factory || !reference)return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    HRESULT hr=initialize(RO_INIT_MULTITHREADED);
    if(FAILED(hr) && hr!=RPC_E_CHANGED_MODE)return hr;
    HSTRING_HEADER header;HSTRING string=NULL;
    hr=reference(name,ARRAYSIZE(name)-1,&header,&string);
    if(SUCCEEDED(hr))hr=factory(string,__uuidof(IGamepadStatics),reinterpret_cast<void **>(&statics));
    return hr;
}

static WORD xinput_buttons(GamepadButtons buttons) {
    static const struct {UINT gamepad;WORD xinput;} map[]={
        {GamepadButtons_DPadUp,XINPUT_GAMEPAD_DPAD_UP},{GamepadButtons_DPadDown,XINPUT_GAMEPAD_DPAD_DOWN},
        {GamepadButtons_DPadLeft,XINPUT_GAMEPAD_DPAD_LEFT},{GamepadButtons_DPadRight,XINPUT_GAMEPAD_DPAD_RIGHT},
        {GamepadButtons_Menu,XINPUT_GAMEPAD_START},{GamepadButtons_View,XINPUT_GAMEPAD_BACK},
        {GamepadButtons_LeftThumbstick,XINPUT_GAMEPAD_LEFT_THUMB},{GamepadButtons_RightThumbstick,XINPUT_GAMEPAD_RIGHT_THUMB},
        {GamepadButtons_LeftShoulder,XINPUT_GAMEPAD_LEFT_SHOULDER},{GamepadButtons_RightShoulder,XINPUT_GAMEPAD_RIGHT_SHOULDER},
        {GamepadButtons_A,XINPUT_GAMEPAD_A},{GamepadButtons_B,XINPUT_GAMEPAD_B},
        {GamepadButtons_X,XINPUT_GAMEPAD_X},{GamepadButtons_Y,XINPUT_GAMEPAD_Y}};
    WORD result=0;
    for(UINT i=0;i<ARRAYSIZE(map);++i)if(static_cast<UINT>(buttons)&map[i].gamepad)result|=map[i].xinput;
    return result;
}
static BYTE trigger_byte(DOUBLE value){return static_cast<BYTE>(value<=0.0?0:value>=1.0?255:value*255.0+0.5);}

static UINT list_pads(void **ids,FM5TriggerInput *inputs,UINT capacity) {
    for(UINT i=0;i<listed_count;++i)listed[i]->Release();
    listed_count=0;
    __FIVectorView_1_Windows__CGaming__CInput__CGamepad *pads=NULL;
    if(!statics || FAILED(statics->get_Gamepads(&pads)) || !pads)return 0;
    unsigned size=0;
    if(FAILED(pads->get_Size(&size)))size=0;
    for(unsigned i=0;i<size && listed_count<capacity && listed_count<LISTED;++i) {
        IGamepad *pad=NULL;IUnknown *identity=NULL;GamepadReading reading={};
        if(FAILED(pads->GetAt(i,&pad)) || !pad)continue;
        if(FAILED(pad->QueryInterface(IID_IUnknown,reinterpret_cast<void **>(&identity))) || !identity){pad->Release();continue;}
        identity->Release();  /* Only its address identifies the pad; listed keeps it alive. */
        if(FAILED(pad->GetCurrentReading(&reading)))reading=GamepadReading();
        listed[listed_count]=pad;identities[listed_count]=identity;
        ids[listed_count]=identity;
        inputs[listed_count].buttons=xinput_buttons(reading.Buttons);
        inputs[listed_count].left=trigger_byte(reading.LeftTrigger);
        inputs[listed_count].right=trigger_byte(reading.RightTrigger);
        ++listed_count;
    }
    pads->Release();
    return listed_count;
}

static HRESULT vibrate(void *id,const float motors[4]) {
    for(UINT i=0;i<listed_count;++i)
        if(identities[i]==id) {
            GamepadVibration vibration={motors[0],motors[1],motors[2],motors[3]};
            return listed[i]->put_Vibration(vibration);
        }
    return HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
}

static BOOL read_xinput(DWORD index,FM5TriggerInput *input) {
    XINPUT_STATE state={};
    if(!xinput_get_state || xinput_get_state(index,&state)!=ERROR_SUCCESS)return FALSE;
    input->buttons=state.Gamepad.wButtons;input->left=state.Gamepad.bLeftTrigger;input->right=state.Gamepad.bRightTrigger;
    return TRUE;
}

extern "C" const FM5TriggerDevice *FM5ImpulseTriggerDevice(void) {
    static const FM5TriggerDevice device={open_device,list_pads,vibrate,read_xinput};
    return &device;
}
