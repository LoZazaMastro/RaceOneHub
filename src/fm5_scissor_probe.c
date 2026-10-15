#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <string.h>
#define CINTERFACE
#pragma warning(push)
#pragma warning(disable:4201)
#include <d3d11.h>
#pragma warning(pop)

typedef void (*SetRects)(void *,UINT,const RECT *);
typedef void (*GetRects)(void *,UINT *,RECT *);
typedef void (*ClearState)(void *);
typedef HRESULT (*CreateDeferred)(void *,UINT,void **);
typedef ULONG (*ReleaseContext)(void *);
static UINT checks;
static const char *mode;
int FM5RunNativeMsaaProbe(void *device);

static void check(BOOL passed,const char *name) {
    ++checks;
    if(!passed) {
        fprintf(stderr,"{\"event\":\"fm5_native_scissor_probe\",\"passed\":false,\"mode\":\"%s\",\"check\":\"%s\"}\n",mode,name);
        fflush(stderr);
        ExitProcess(1);
    }
}

static void run_checks(void *context) {
    void **methods=(void **)((BYTE *)context+0x20);
    SetRects set=(SetRects)methods[46];
    GetRects get=(GetRects)methods[97];
    ClearState clear=(ClearState)methods[111];
    const RECT expected[2]={{3,5,61,77},{7,11,101,131}};
    RECT output[4],zero[4]={0},before[4];
    UINT count=0;
    get(context,&count,NULL);
    check(count==0,"initial_count");
    set(context,2,expected);
    count=0xffffffff;
    get(context,&count,NULL);
    check(count==2,"published_set_count");
    memset(output,0xcd,sizeof(output));
    memcpy(before,output,sizeof(before));
    count=1;
    get(context,&count,output);
    check(count==1,"short_capacity_count");
    check(!memcmp(output,expected,sizeof(RECT)),"short_capacity_rectangle");
    check(!memcmp(output+1,before+1,sizeof(output)-sizeof(RECT)),"short_capacity_canaries");
    count=4;
    get(context,&count,output);
    check(count==2,"full_count");
    check(!memcmp(output,expected,sizeof(expected)),"real_native_rectangles");
    check(!memcmp(output+2,zero+2,sizeof(RECT)*2),"unused_output_zeroed");
    set(context,0,NULL);
    memset(output,0xcd,sizeof(output));
    count=4;
    get(context,&count,output);
    check(count==0 && !memcmp(output,zero,sizeof(output)),"zero_rectangles");
    set(context,2,expected);
    clear(context);
    count=4;
    memset(output,0xcd,sizeof(output));
    get(context,&count,output);
    check(count==0 && !memcmp(output,zero,sizeof(output)),"clear_state_applied");
}

static void run_sampler_checks(void *device,void *context) {
    typedef HRESULT (*CreateSampler)(void *,const D3D11_SAMPLER_DESC *,void **);
    typedef void (*SetSamplers)(void *,UINT,UINT,void *const *);
    typedef void (*GetSamplers)(void *,UINT,UINT,void **);
    void **methods=(void **)((BYTE*)context+0x20);
    SetSamplers set=(SetSamplers)methods[11];
    GetSamplers get=(GetSamplers)methods[76];
    D3D11_SAMPLER_DESC desc={0};void *sampler=NULL,*out[4],*binding[2];
    desc.Filter=D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    desc.AddressU=desc.AddressV=desc.AddressW=D3D11_TEXTURE_ADDRESS_CLAMP;
    desc.MaxAnisotropy=1;desc.ComparisonFunc=D3D11_COMPARISON_ALWAYS;desc.MaxLOD=D3D11_FLOAT32_MAX;
    check(SUCCEEDED(((CreateSampler)(*(void***)device)[23])(device,&desc,&sampler))&&sampler!=NULL,"create_real_sampler");
    binding[0]=sampler;binding[1]=NULL;set(context,1,2,binding);
    get(context,0,4,out);
    check(out[0]==NULL&&out[1]==sampler&&out[2]==NULL&&out[3]==NULL,"real_sampler_cache_identity_and_nulls");
    ((ReleaseContext)(*(void***)out[1])[2])(out[1]);
    ULONG before=*(volatile ULONG*)((BYTE*)sampler+8);
    get(context,1,1,out);
    check(out[0]==sampler&&*(volatile ULONG*)((BYTE*)sampler+8)==before+1,"sampler_get_addref");
    ((ReleaseContext)(*(void***)out[0])[2])(out[0]);
    check(*(volatile ULONG*)((BYTE*)sampler+8)==before,"sampler_get_owned_reference_release");
    get(context,1,1,NULL);
    check(*(volatile ULONG*)((BYTE*)sampler+8)==before,"null_sampler_output_no_reference");
    ((ClearState)methods[111])(context);
    get(context,0,4,out);
    check(out[0]==NULL&&out[1]==NULL&&out[2]==NULL&&out[3]==NULL,"clear_state_clears_actual_samplers");
    ((ReleaseContext)(*(void***)sampler)[2])(sampler);
}

void FM5RunNativeScissorProbe(void *device,void *context) {
    WCHAR enabled[2]={0};
    if(GetEnvironmentVariableW(L"FM5_NATIVE_SCISSOR_PROBE",enabled,2)!=1 || enabled[0]!=L'1')return;
    mode="immediate";
    run_checks(context);
    run_sampler_checks(device,context);
    mode="deferred";
    void *deferred=NULL;
    CreateDeferred create=(CreateDeferred)(*(void ***)device)[27];
    check(SUCCEEDED(create(device,0,&deferred)) && deferred!=NULL,"create_deferred");
    run_checks(deferred);
    run_sampler_checks(device,deferred);
    ((ReleaseContext)(*(void ***)deferred)[2])(deferred);
    if (GetEnvironmentVariableW(L"FM5_NATIVE_MSAA4_PROBE",enabled,2)==1 && enabled[0]==L'1')
        check(FM5RunNativeMsaaProbe(device),"native_msaa4_resource_and_shader");
    fprintf(stderr,"{\"event\":\"fm5_native_scissor_probe\",\"passed\":true,\"checks\":%u}\n",checks);
    fflush(stderr);
    ExitProcess(0);
}
