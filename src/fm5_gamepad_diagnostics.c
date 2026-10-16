#define WIN32_LEAN_AND_MEAN
#include "fm5_gamepad_diagnostics.h"
#include "fm5_impulse_triggers.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef struct Backend {
    HRESULT (WINAPI *set)(void *,const float *);
    HWND (WINAPI *foreground)(void);
    void **vtable;
    volatile DWORD *mask634,*mask638,*cache;
    HWND volatile *owner;
    volatile BYTE *bypass;
} Backend;
typedef struct Snapshot {DWORD index,mask634,mask638,cache;HWND owner,foreground;BYTE bypass;} Snapshot;
static INIT_ONCE diagnostics_once=INIT_ONCE_STATIC_INIT;
static BOOL diagnostics_enabled;
static SRWLOCK log_lock=SRWLOCK_INIT;
static UINT log_count;
static ULONGLONG last_log;
static BOOL seen[4],previous_nonzero[4];
static float previous[4][4];
#ifdef FM5_GAMEPAD_DIAGNOSTICS_TEST
static Backend test_backend;
static ULONGLONG test_ticks;
void FM5GamepadSetTestTime(ULONGLONG ticks){test_ticks=ticks;}
void FM5GamepadSetTestBackend(const FM5GamepadTestBackend *backend) {
    C_ASSERT(sizeof(Backend)==sizeof(FM5GamepadTestBackend));
    if(backend)memcpy(&test_backend,backend,sizeof(test_backend));else memset(&test_backend,0,sizeof(test_backend));
}
#endif
static BOOL CALLBACK configure(PINIT_ONCE once,PVOID parameter,PVOID *context) {
    WCHAR value[4]={0};(void)once;(void)parameter;(void)context;
    diagnostics_enabled=GetEnvironmentVariableW(L"FM5_GAMEPAD_DIAGNOSTICS",value,4)==1 && value[0]==L'1';
    return TRUE;
}
#ifndef FM5_GAMEPAD_DIAGNOSTICS_TEST
/* The prepared host, recognized by its Gamepad vtable and native SetVibration. */
static BYTE *host_gamepad(void) {
    static const BYTE signature[32]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x48,0x89,0x68,0x20,0x56,0x57,0x41,0x56,0x48,0x81,0xec,0xb0,0x00,0x00,0x00,0x0f,0x29,0x70,0xd8,0x0f,0x29,0x78,0xc8,0x44,0x0f};
    static const DWORD methods[10]={0xc1ef0,0xc1ee0,0xc1ea0,0xc1e10,0xc1d50,0xc1d30,0,0xbf000,0xbf0d0,0xbf120};
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!base)return NULL;
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)base;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0x40 || dos->e_lfanew>0x1000)return NULL;
    IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64 *)(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt->OptionalHeader.AddressOfEntryPoint!=0x2226fc || nt->OptionalHeader.SizeOfImage<0x37d000 || (nt->OptionalHeader.SizeOfImage>0x3c9000 && nt->OptionalHeader.SizeOfImage!=0x3de000) || memcmp(base+0xbed60,signature,sizeof(signature)))return NULL;
    void **vtable=(void **)(base+0x2a9278);
    /* Slot6 is redirected to FM5SetGamepadVibration, by the host's import thunk or at load. */
    for(UINT i=0;i<10;++i)if(i!=6 && vtable[i]!=base+methods[i])return NULL;
    return base;
}
#endif
static HRESULT backend(void *self,Backend *out) {
#ifdef FM5_GAMEPAD_DIAGNOSTICS_TEST
    *out=test_backend;
#else
    BYTE *base=host_gamepad();
    if(!base)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    out->vtable=(void **)(base+0x2a9278);
    out->set=(HRESULT (WINAPI *)(void *,const float *))(uintptr_t)(base+0xbed60);
    out->foreground=GetForegroundWindow;
    out->mask634=(volatile DWORD *)(base+0x32b634);out->mask638=(volatile DWORD *)(base+0x32b638);
    out->cache=(volatile DWORD *)(base+0x32b610);out->owner=(HWND volatile *)(base+0x32b5b0);out->bypass=base+0x32b640;
#endif
    if(!self)return E_POINTER;
    if(!out->set || !out->foreground || !out->vtable || !out->mask634 || !out->mask638 || !out->cache || !out->owner || !out->bypass || *(void ***)self!=out->vtable)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    return S_OK;
}
static Snapshot snapshot(const Backend *b,void *self) {
    Snapshot s;memcpy(&s.index,(const BYTE *)self+0x20,4);
    s.mask634=*b->mask634;s.mask638=*b->mask638;s.owner=*b->owner;s.foreground=b->foreground();s.bypass=*b->bypass;
    s.cache=s.index<4?b->cache[s.index]:0;return s;
}
static BOOL log_requested(DWORD index,const float request[4],BOOL has_request,UINT *sequence) {
#ifdef FM5_GAMEPAD_DIAGNOSTICS_TEST
    ULONGLONG now=test_ticks;
#else
    ULONGLONG now=GetTickCount64();
#endif
    BOOL nonzero=FALSE,changed=FALSE,emit=FALSE;
    if(has_request)for(UINT i=0;i<4;++i)if(request[i]!=0.0f)nonzero=TRUE;
    AcquireSRWLockExclusive(&log_lock);
    if(index<4){changed=!seen[index] || previous_nonzero[index]!=nonzero || (nonzero && has_request && memcmp(previous[index],request,16));seen[index]=TRUE;previous_nonzero[index]=nonzero;if(has_request)memcpy(previous[index],request,16);}
    else changed=TRUE;
    if(log_count<128 && (log_count<16 || (changed && now-last_log>=1000))){*sequence=++log_count;last_log=now;emit=TRUE;}
    ReleaseSRWLockExclusive(&log_lock);return emit;
}
/* Requests the native setter accepted carry trigger levels it does not send. */
static void forward_triggers(const Backend *b,void *self,const float *vibration,HRESULT hr) {
    DWORD index;
    if(hr!=S_OK || !vibration)return;
    memcpy(&index,(const BYTE *)self+0x20,4);
    FM5TriggerHost host={b->mask634,b->cache,b->owner,b->bypass,b->foreground};
    FM5ImpulseTriggersRequest(&host,index,vibration);
}
__declspec(dllexport) HRESULT WINAPI FM5SetGamepadVibration(void *self,const float *vibration) {
    Backend b;HRESULT hr=backend(self,&b);if(FAILED(hr))return hr;
    InitOnceExecuteOnce(&diagnostics_once,configure,NULL,NULL);
    if(!diagnostics_enabled){hr=b.set(self,vibration);forward_triggers(&b,self,vibration,hr);return hr;}
    float request[4]={0};if(vibration)memcpy(request,vibration,sizeof(request));
    Snapshot before=snapshot(&b,self);
    /* Never override focus, gate bits, amplitudes, routing or the native HRESULT. */
    hr=b.set(self,vibration);
    forward_triggers(&b,self,vibration,hr);
    Snapshot after=snapshot(&b,self);UINT sequence=0;
    if(log_requested(before.index,request,vibration!=NULL,&sequence)) {
        fprintf(stderr,"FM5_GAMEPAD_VIBRATION seq=%u thread=%lu index=%lu request_ptr=%p request=[%.9g,%.9g,%.9g,%.9g] mask634=0x%lx mask638=0x%lx owner=%p foreground=%p bypass=%u pre_cache=0x%08lx post_cache=0x%08lx hr=0x%08lx post_owner=%p post_foreground=%p\n",sequence,GetCurrentThreadId(),before.index,(const void *)vibration,(double)request[0],(double)request[1],(double)request[2],(double)request[3],before.mask634,before.mask638,(void *)before.owner,(void *)before.foreground,(UINT)before.bypass,before.cache,after.cache,(unsigned long)hr,(void *)after.owner,(void *)after.foreground);
        fflush(stderr);
    }
    return hr;
}
/* The released host leaves slot 6 on the native setter; route it here at load,
   before guest code runs, on the exact host only. */
BOOL FM5InstallGamepadVibrationHook(void) {
    InitOnceExecuteOnce(&diagnostics_once,configure,NULL,NULL);
    if(!diagnostics_enabled && !FM5ImpulseTriggersEnabled())return FALSE;
#ifdef FM5_GAMEPAD_DIAGNOSTICS_TEST
    void **vtable=test_backend.vtable,*native=(void *)(uintptr_t)test_backend.set;
    if(!vtable || !native)return FALSE;
#else
    BYTE *base=host_gamepad();
    if(!base)return FALSE;
    void **vtable=(void **)(base+0x2a9278),*native=base+0xbed60;
#endif
    void *hook=(void *)(uintptr_t)FM5SetGamepadVibration;DWORD old;
    if(vtable[6]!=native)return vtable[6]==hook;  /* A host built with the redirect keeps its thunk. */
    if(!VirtualProtect(&vtable[6],sizeof(void *),PAGE_READWRITE,&old))return FALSE;
    InterlockedCompareExchangePointer(&vtable[6],hook,native);
    VirtualProtect(&vtable[6],sizeof(void *),old,&old);
    return vtable[6]==hook;
}
