#include "../../fm5_gamepad_diagnostics.h"
#include "../../fm5_impulse_triggers.h"
#include <stdio.h>
#include <string.h>
typedef struct Pad {void **vtable;BYTE reserved[24];DWORD index;} Pad;
static void *table[10];
static DWORD mask634=1,mask638=0x1234,cache[4];
static HWND owner=(HWND)0x12345678;
static BYTE bypass;
static UINT calls,foreground_calls,checks;
static const float *last_request;
static void *last_self;
static HRESULT native_result=S_OK;
static HRESULT WINAPI native_set(void *self,const float *request) {
    last_self=self;last_request=request;++calls;
    cache[0]=0x12345678;
    return native_result;
}
static HWND WINAPI foreground(void){++foreground_calls;return(HWND)(ULONG_PTR)0xabcdef01;}
#define CHECK(x) do{++checks;if(!(x)){printf("failed_line=%u\n",__LINE__);return 1;}}while(0)
int main(void) {
    C_ASSERT(offsetof(Pad,index)==0x20);
    Pad pad={table,{0},0};float request[4]={0.1f,0.2f,0.3f,0.4f},copy[4];memcpy(copy,request,sizeof(copy));
    FM5GamepadTestBackend b={native_set,foreground,table,&mask634,&mask638,cache,&owner,&bypass};FM5GamepadSetTestBackend(&b);
    CHECK(FM5SetGamepadVibration(&pad,request)==S_OK && last_self==&pad && last_request==request && calls==1);
    CHECK(!memcmp(request,copy,sizeof(copy)) && mask634==1 && mask638==0x1234 && !bypass && owner==(HWND)0x12345678 && cache[0]==0x12345678);
    WCHAR impulse[4]={0};BOOL forwarding=!(GetEnvironmentVariableW(L"FM5_IMPULSE_TRIGGERS",impulse,4)==1 && impulse[0]==L'0');
    float left=-1,right=-1;
    /* Accepted requests hand their trigger levels on; rejected ones do not. */
    CHECK(FM5ImpulseTriggersWanted(0,&left,&right)==forwarding && (forwarding?left==0.3f && right==0.4f:left==0 && right==0));
    float rejected[4]={0.1f,0.2f,0.9f,0.9f};
    native_result=E_INVALIDARG;CHECK(FM5SetGamepadVibration(&pad,rejected)==E_INVALIDARG && calls==2 && last_request==rejected);
    CHECK(FM5ImpulseTriggersWanted(0,&left,&right)==forwarding && (forwarding?left==0.3f && right==0.4f:left==0 && right==0));
    native_result=S_FALSE;CHECK(FM5SetGamepadVibration(&pad,NULL)==S_FALSE && calls==3 && !last_request);
    CHECK(FM5SetGamepadVibration(NULL,request)==E_POINTER && calls==3);
    Pad invalid=pad;invalid.vtable=NULL;CHECK(FM5SetGamepadVibration(&invalid,request)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && calls==3);
    native_result=S_OK;for(UINT i=0;i<400;++i){request[0]=(i&1)?0.8f:0.0f;FM5GamepadSetTestTime((ULONGLONG)i*1000);CHECK(FM5SetGamepadVibration(&pad,request)==S_OK && last_self==&pad && last_request==request);}
    CHECK(calls==403);
    WCHAR env[4]={0};BOOL enabled=GetEnvironmentVariableW(L"FM5_GAMEPAD_DIAGNOSTICS",env,4)==1 && env[0]==L'1';
    /* Slot 6 moves from the native setter to the hook when either feature is on, once. */
    table[6]=(void *)(ULONG_PTR)native_set;
    CHECK(FM5InstallGamepadVibrationHook()==(enabled || forwarding) && table[6]==((enabled || forwarding)?(void *)(ULONG_PTR)FM5SetGamepadVibration:(void *)(ULONG_PTR)native_set));
    CHECK(FM5InstallGamepadVibrationHook()==(enabled || forwarding));
    table[6]=(void *)(ULONG_PTR)foreground;CHECK(!FM5InstallGamepadVibrationHook() && table[6]==(void *)(ULONG_PTR)foreground);
    FM5GamepadSetTestBackend(NULL);CHECK(FM5SetGamepadVibration(&pad,request)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && calls==403);
    CHECK(!FM5InstallGamepadVibrationHook());
    CHECK(enabled?foreground_calls==calls*2:foreground_calls==0);
    printf("{\"passed\":true,\"checks\":%u,\"native_calls\":%u,\"foreground_calls\":%u,\"enabled\":%s,\"forwarding\":%s}\n",checks,calls,foreground_calls,enabled?"true":"false",forwarding?"true":"false");return 0;
}
