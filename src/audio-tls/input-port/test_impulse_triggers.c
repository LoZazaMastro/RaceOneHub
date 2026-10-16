#include "../../fm5_impulse_triggers.h"
#include <math.h>
#include <stdio.h>
#include <string.h>
typedef struct Call {void *pad;float motors[4];} Call;
static Call calls[256];
static UINT call_count,checks;
static void *pad_ids[3]={(void *)(ULONG_PTR)0x1000,(void *)(ULONG_PTR)0x2000,(void *)(ULONG_PTR)0x3000};
static FM5TriggerInput pad_inputs[3],live[4];
static UINT pad_count=1;
static HRESULT vibrate_result=S_OK;
static DWORD connected=1,applied[4];
static HWND owner=(HWND)(ULONG_PTR)0x100,front=(HWND)(ULONG_PTR)0x100;
static BYTE bypass;
static HRESULT fake_open(void){return S_OK;}
static UINT fake_pads(void **ids,FM5TriggerInput *inputs,UINT capacity) {
    UINT n=pad_count<capacity?pad_count:capacity;
    for(UINT i=0;i<n;++i){ids[i]=pad_ids[i];inputs[i]=pad_inputs[i];}
    return n;
}
static HRESULT fake_vibrate(void *id,const float motors[4]) {
    if(call_count<256){calls[call_count].pad=id;memcpy(calls[call_count].motors,motors,sizeof(calls[0].motors));}
    ++call_count;return vibrate_result;
}
static BOOL fake_xinput(DWORD index,FM5TriggerInput *input){if(index>=4)return FALSE;*input=live[index];return TRUE;}
static HWND WINAPI foreground(void){return front;}
static BOOL last(UINT count,void *pad,float m0,float m1,float left,float right) {
    const Call *c=&calls[count-1];
    return count==call_count && c->pad==pad && fabsf(c->motors[0]-m0)<1e-4f && fabsf(c->motors[1]-m1)<1e-4f && c->motors[2]==left && c->motors[3]==right;
}
#define CHECK(x) do{++checks;if(!(x)){printf("failed_line=%u\n",__LINE__);return 1;}}while(0)
int main(int argc,char **argv) {
    static const FM5TriggerDevice device={fake_open,fake_pads,fake_vibrate,fake_xinput};
    FM5TriggerHost host={&connected,applied,&owner,&bypass,foreground};
    float rest[4]={0},drive[4]={1.0f,0.5f,0.25f,0.75f},motors_only[4]={1.0f,1.0f,0,0},left,right;
    if(argc==2 && !strcmp(argv[1],"--log")) {
        /* Diagnostics budget: the first 16 records, then at most one a second. */
        FM5ImpulseTriggersSetTest(&device,TRUE,TRUE);
        for(UINT i=0;i<200;++i){drive[2]=(i&1)?0.5f:0.25f;FM5ImpulseTriggersRequest(&host,0,drive);FM5ImpulseTriggersStep((ULONGLONG)i*10);}
        printf("{\"passed\":true,\"vibrate_calls\":%u}\n",call_count);return 0;
    }
    FM5ImpulseTriggersSetTest(&device,TRUE,FALSE);
    /* Nothing to forward: silence stays silent, and XInput keeps the main motors. */
    FM5ImpulseTriggersRequest(&host,0,rest);CHECK(!FM5ImpulseTriggersWanted(0,NULL,NULL) && !FM5ImpulseTriggersStep(0) && !call_count);
    FM5ImpulseTriggersRequest(&host,0,motors_only);CHECK(!FM5ImpulseTriggersStep(0) && !call_count);
    /* Triggers forward with the motor words XInput already has (low word left). */
    applied[0]=0x8000ffff;
    FM5ImpulseTriggersRequest(&host,0,drive);CHECK(FM5ImpulseTriggersWanted(0,&left,&right) && left==0.25f && right==0.75f);
    CHECK(FM5ImpulseTriggersStep(0) && last(1,pad_ids[0],1.0f,32768.0f/65535.0f,0.25f,0.75f));
    CHECK(FM5ImpulseTriggersStep(0) && call_count==1);
    /* A repeated request is sent again, in case an XInput write reset the triggers. */
    FM5ImpulseTriggersRequest(&host,0,drive);CHECK(FM5ImpulseTriggersStep(0) && last(2,pad_ids[0],1.0f,32768.0f/65535.0f,0.25f,0.75f));
    /* The native gates: window in front, no bypass, pad connected, an owner window. */
    front=(HWND)(ULONG_PTR)0x200;applied[0]=0;
    CHECK(FM5ImpulseTriggersStep(0) && last(3,pad_ids[0],0,0,0,0));
    CHECK(FM5ImpulseTriggersStep(0) && call_count==3);
    front=owner;CHECK(FM5ImpulseTriggersStep(0) && last(4,pad_ids[0],0,0,0.25f,0.75f));
    bypass=1;CHECK(FM5ImpulseTriggersStep(0) && last(5,pad_ids[0],0,0,0,0));
    bypass=0;CHECK(FM5ImpulseTriggersStep(0) && last(6,pad_ids[0],0,0,0.25f,0.75f));
    connected=0;CHECK(FM5ImpulseTriggersStep(0) && last(7,pad_ids[0],0,0,0,0));
    connected=1;owner=NULL;front=NULL;CHECK(FM5ImpulseTriggersStep(0) && call_count==7);
    owner=front=(HWND)(ULONG_PTR)0x100;CHECK(FM5ImpulseTriggersStep(0) && last(8,pad_ids[0],0,0,0.25f,0.75f));
    /* Stopping is sent once, then the worker goes idle. */
    FM5ImpulseTriggersRequest(&host,0,rest);CHECK(FM5ImpulseTriggersWanted(0,NULL,NULL) && !FM5ImpulseTriggersStep(0) && last(9,pad_ids[0],0,0,0,0));
    FM5ImpulseTriggersRequest(&host,0,rest);CHECK(!FM5ImpulseTriggersWanted(0,NULL,NULL) && !FM5ImpulseTriggersStep(0) && call_count==9);
    /* No pad listed yet: keep the request and send it when one appears. */
    pad_count=0;FM5ImpulseTriggersRequest(&host,0,drive);CHECK(FM5ImpulseTriggersStep(0) && call_count==9);
    pad_count=1;CHECK(FM5ImpulseTriggersStep(0) && last(10,pad_ids[0],0,0,0.25f,0.75f));
    /* A failed send is retried while the request stands. */
    vibrate_result=E_FAIL;FM5ImpulseTriggersRequest(&host,0,drive);CHECK(FM5ImpulseTriggersStep(0) && call_count==11);
    CHECK(FM5ImpulseTriggersStep(0) && call_count==12);
    vibrate_result=S_OK;CHECK(FM5ImpulseTriggersStep(0) && last(13,pad_ids[0],0,0,0.25f,0.75f));
    FM5ImpulseTriggersRequest(&host,0,rest);CHECK(!FM5ImpulseTriggersStep(0) && last(14,pad_ids[0],0,0,0,0));
    /* Several pads: the one whose live input matches XInput's for the guest index. */
    pad_count=2;connected=0x6;
    live[1].buttons=0x1000;live[1].left=200;pad_inputs[1].buttons=0x1000;pad_inputs[1].left=190;
    float brake[4]={0,0,0.9f,0};
    FM5ImpulseTriggersRequest(&host,1,brake);CHECK(FM5ImpulseTriggersStep(0) && last(15,pad_ids[1],0,0,0.9f,0));
    memset(live,0,sizeof(live));memset(pad_inputs,0,sizeof(pad_inputs));
    FM5ImpulseTriggersRequest(&host,1,brake);CHECK(FM5ImpulseTriggersStep(0) && last(16,pad_ids[1],0,0,0.9f,0));
    /* Idle and alike: no guess for a guest index never matched. */
    FM5ImpulseTriggersRequest(&host,2,brake);CHECK(FM5ImpulseTriggersStep(0) && call_count==16);
    live[2].right=255;pad_inputs[0].right=250;
    FM5ImpulseTriggersRequest(&host,2,brake);CHECK(FM5ImpulseTriggersStep(0) && last(17,pad_ids[0],0,0,0.9f,0));
    FM5ImpulseTriggersRequest(&host,1,rest);FM5ImpulseTriggersRequest(&host,2,rest);
    CHECK(!FM5ImpulseTriggersStep(0) && calls[17].pad==pad_ids[1] && last(19,pad_ids[0],0,0,0,0));
    /* Indexes past the host's four pads, and disabled forwarding, are ignored. */
    FM5ImpulseTriggersRequest(&host,4,drive);CHECK(!FM5ImpulseTriggersWanted(4,NULL,NULL) && !FM5ImpulseTriggersStep(0));
    FM5ImpulseTriggersSetTest(&device,FALSE,FALSE);
    FM5ImpulseTriggersRequest(&host,0,drive);CHECK(!FM5ImpulseTriggersWanted(0,&left,&right) && left==0 && right==0 && !FM5ImpulseTriggersStep(0));
    printf("{\"passed\":true,\"checks\":%u,\"vibrate_calls\":%u}\n",checks,call_count);return 0;
}
