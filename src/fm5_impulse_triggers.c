#define WIN32_LEAN_AND_MEAN
#include "fm5_impulse_triggers.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The host's native setter sends only the two main motors to XInput and drops
   the trigger levels. A worker thread forwards them through
   Windows.Gaming.Input, with the motor values XInput already has, under the
   native gates: pad connected, rumble not bypassed and the game window in
   front. As natively, stopping is never gated. */
enum {PADS=4,DEVICES=8,WATCH_MS=100,MATCH=24};
typedef struct Wanted {float left,right;BOOL pending;} Wanted;
typedef struct Applied {void *pad;float left,right;DWORD motors;BOOL active,stale;} Applied;
static SRWLOCK lock=SRWLOCK_INIT;
static FM5TriggerHost host;
static BOOL have_host;
static Wanted wanted[PADS];
static Applied applied[PADS];
static void *mapped[PADS];
static INIT_ONCE config_once=INIT_ONCE_STATIC_INIT;
static BOOL enabled,logging;
static const FM5TriggerDevice *device;
static UINT log_count;
static ULONGLONG last_log;
#ifndef FM5_IMPULSE_TRIGGERS_TEST
static INIT_ONCE worker_once=INIT_ONCE_STATIC_INIT;
static HANDLE wake;
#endif
static BOOL CALLBACK configure(PINIT_ONCE once,PVOID parameter,PVOID *context) {
    WCHAR value[4]={0};(void)once;(void)parameter;(void)context;
    enabled=!(GetEnvironmentVariableW(L"FM5_IMPULSE_TRIGGERS",value,4)==1 && value[0]==L'0');
    value[0]=0;logging=GetEnvironmentVariableW(L"FM5_GAMEPAD_DIAGNOSTICS",value,4)==1 && value[0]==L'1';
    return TRUE;
}
BOOL FM5ImpulseTriggersEnabled(void) {
    InitOnceExecuteOnce(&config_once,configure,NULL,NULL);
    return enabled;
}
static void report(DWORD index,UINT pads,void *pad,const float values[4],BOOL gate,HRESULT hr,ULONGLONG now) {
    if(!logging || log_count>=128 || (log_count>=16 && now-last_log<1000))return;
    ++log_count;last_log=now;
    fprintf(stderr,"FM5_IMPULSE_TRIGGERS seq=%u index=%lu pads=%u pad=%p motors=[%.3f,%.3f] triggers=[%.3f,%.3f] gate=%u hr=0x%08lx\n",log_count,index,pads,pad,(double)values[0],(double)values[1],(double)values[2],(double)values[3],(UINT)gate,(unsigned long)hr);
    fflush(stderr);
}
static void *choose(DWORD index,void *const *ids,const FM5TriggerInput *inputs,UINT count) {
    FM5TriggerInput live;void *match=NULL;UINT matches=0;
    if(!count)return NULL;
    if(count==1)return mapped[index]=ids[0];
    /* Several pads: the one whose input matches XInput's for this index. Trigger
       rumble answers a held trigger, so the pad in use is rarely idle here. */
    if(device->xinput && device->xinput(index,&live) && (live.buttons || live.left>MATCH || live.right>MATCH))
        for(UINT i=0;i<count;++i)
            if(inputs[i].buttons==live.buttons && abs(inputs[i].left-live.left)<=MATCH && abs(inputs[i].right-live.right)<=MATCH){match=ids[i];++matches;}
    if(matches==1)return mapped[index]=match;
    for(UINT i=0;i<count;++i)if(ids[i]==mapped[index])return ids[i];
    return NULL;
}
static BOOL step(ULONGLONG now) {
    Wanted next[PADS];FM5TriggerHost h;BOOL have,listed=FALSE,busy=FALSE;
    void *ids[DEVICES];FM5TriggerInput inputs[DEVICES];UINT count=0;
    AcquireSRWLockExclusive(&lock);
    memcpy(next,wanted,sizeof(next));for(UINT i=0;i<PADS;++i)wanted[i].pending=FALSE;
    h=host;have=have_host;
    ReleaseSRWLockExclusive(&lock);
    if(!have || !device)return FALSE;
    for(DWORD i=0;i<PADS;++i) {
        BOOL requested=next[i].left!=0.0f || next[i].right!=0.0f,gate;
        float left,right,values[4];DWORD motors;void *pad;HRESULT hr;
        if(!requested && !applied[i].active)continue;
        gate=(*h.connected&(1u<<i))!=0;
        if(gate && requested){HWND owner=*h.owner;gate=!*h.bypass && owner && h.foreground()==owner;}
        busy|=requested;  /* Keep watching: the gate may open again without a new request. */
        left=gate?next[i].left:0.0f;right=gate?next[i].right:0.0f;motors=h.applied[i];
        if(left==0.0f && right==0.0f && !applied[i].active)continue;
        if(applied[i].active && !applied[i].stale && !next[i].pending && left==applied[i].left && right==applied[i].right && motors==applied[i].motors)continue;
        if(!listed){count=device->pads(ids,inputs,DEVICES);listed=TRUE;}
        pad=choose(i,ids,inputs,count);
        values[0]=LOWORD(motors)/65535.0f;values[1]=HIWORD(motors)/65535.0f;values[2]=left;values[3]=right;
        hr=pad?device->vibrate(pad,values):HRESULT_FROM_WIN32(ERROR_DEVICE_NOT_CONNECTED);
        report(i,count,pad,values,gate,hr,now);
        if(SUCCEEDED(hr)){applied[i].pad=pad;applied[i].left=left;applied[i].right=right;applied[i].motors=motors;applied[i].active=left!=0.0f || right!=0.0f;applied[i].stale=FALSE;}
        else if(left==0.0f && right==0.0f)applied[i].active=FALSE;  /* Nothing left to stop on a pad that is gone. */
        else applied[i].stale=TRUE;  /* Retried while the request stands. */
    }
    for(UINT i=0;i<PADS;++i)busy|=applied[i].active;
    return busy;
}
#ifndef FM5_IMPULSE_TRIGGERS_TEST
static DWORD WINAPI worker(LPVOID parameter) {
    void *ids[DEVICES];FM5TriggerInput inputs[DEVICES];HRESULT hr;(void)parameter;
    hr=device->open();
    if(FAILED(hr)){
        if(logging){fprintf(stderr,"FM5_IMPULSE_TRIGGERS unavailable hr=0x%08lx\n",(unsigned long)hr);fflush(stderr);}
        return 0;
    }
    device->pads(ids,inputs,DEVICES);  /* Windows.Gaming.Input finds pads after the first query. */
    for(;;)WaitForSingleObject(wake,step(GetTickCount64())?WATCH_MS:INFINITE);
}
static BOOL CALLBACK start(PINIT_ONCE once,PVOID parameter,PVOID *context) {
    HANDLE thread;(void)once;(void)parameter;(void)context;
    device=FM5ImpulseTriggerDevice();
    wake=CreateEventW(NULL,FALSE,FALSE,NULL);
    if(wake && (thread=CreateThread(NULL,0,worker,NULL,0,NULL))!=NULL)CloseHandle(thread);
    return TRUE;
}
#endif
void FM5ImpulseTriggersRequest(const FM5TriggerHost *source,DWORD index,const float request[4]) {
    BOOL signal;
    if(!source || !request || index>=PADS || !FM5ImpulseTriggersEnabled())return;
    AcquireSRWLockExclusive(&lock);
    if(!have_host){host=*source;have_host=TRUE;}
    signal=request[2]!=0.0f || request[3]!=0.0f || wanted[index].left!=0.0f || wanted[index].right!=0.0f;
    wanted[index].left=request[2];wanted[index].right=request[3];wanted[index].pending|=signal;
    ReleaseSRWLockExclusive(&lock);
#ifndef FM5_IMPULSE_TRIGGERS_TEST
    InitOnceExecuteOnce(&worker_once,start,NULL,NULL);
    if(signal && wake)SetEvent(wake);
#endif
}
#ifdef FM5_IMPULSE_TRIGGERS_TEST
void FM5ImpulseTriggersSetTest(const FM5TriggerDevice *test_device,BOOL test_enabled,BOOL test_logging) {
    InitOnceExecuteOnce(&config_once,configure,NULL,NULL);
    device=test_device;enabled=test_enabled;logging=test_logging;
    AcquireSRWLockExclusive(&lock);memset(wanted,0,sizeof(wanted));have_host=FALSE;ReleaseSRWLockExclusive(&lock);
    memset(applied,0,sizeof(applied));memset(mapped,0,sizeof(mapped));log_count=0;last_log=0;
}
BOOL FM5ImpulseTriggersStep(ULONGLONG now){return step(now);}
BOOL FM5ImpulseTriggersWanted(DWORD index,float *left,float *right) {
    BOOL pending=FALSE;
    if(index>=PADS)return FALSE;
    AcquireSRWLockExclusive(&lock);
    if(left)*left=wanted[index].left;
    if(right)*right=wanted[index].right;
    pending=wanted[index].pending;
    ReleaseSRWLockExclusive(&lock);
    return pending;
}
#endif
