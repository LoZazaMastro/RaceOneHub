#include "fm5_performance_device.h"
#include <stdio.h>
#include <string.h>

typedef HRESULT (WINAPI *QI)(void *, REFIID, void **);
typedef ULONG (WINAPI *Ref)(void *);
typedef void (WINAPI *Unknown)(void *, ...);
typedef struct NativeVT { QI query; Ref add; Ref release; void *methods[62]; } NativeVT;
typedef struct Native { NativeVT *vt; LONG refs; LONG queries; LONG releases; } Native;
typedef struct DeviceVT {
    QI query; Ref add; Ref release; Unknown unused[51];
    HRESULT (WINAPI *feature)(void *,UINT32,UINT32);
    HRESULT (WINAPI *create)(void *,const FM5ScratchDescription *,void **);
    Unknown extended[9];
} DeviceVT;
typedef struct ScratchVT {
    QI query; Ref add; Ref release;
    void (WINAPI *get_device)(void *,void **);
    HRESULT (WINAPI *get_private)(void *,REFGUID,UINT *,void *);
    HRESULT (WINAPI *set_private)(void *,REFGUID,UINT,const void *);
    HRESULT (WINAPI *set_interface)(void *,REFGUID,void *);
    HRESULT (WINAPI *set_interface2)(void *,REFGUID,void *);
    UINT (WINAPI *get_type)(void *);
} ScratchVT;
static GUID iid_unknown = {0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static GUID key = {0x12345678,0x1234,0x1234,{1,2,3,4,5,6,7,8}};
static GUID iid_device = {0xdb6f6ddb,0xac77,0x4e88,{0x82,0x53,0x81,0x9d,0xf9,0xbb,0xf1,0x40}};
static HRESULT WINAPI nq(void *self,REFIID iid,void **out) {
    ++((Native *)self)->queries; *out=NULL;
    if (!memcmp(iid,&iid_device,sizeof(GUID))) { InterlockedIncrement(&((Native *)self)->refs); *out=self; return S_OK; }
    return E_NOINTERFACE;
}
static ULONG WINAPI na(void *self) { return (ULONG)InterlockedIncrement(&((Native *)self)->refs); }
static ULONG WINAPI nr(void *self) { InterlockedIncrement(&((Native *)self)->releases); return (ULONG)InterlockedDecrement(&((Native *)self)->refs); }
static NativeVT nvt = {nq,na,nr,{0}};
static Native context;
static LONG wrap_calls;
static BOOL wrap_fail;
static UINT last_flags;
static BOOL native_context_fail;
static UINT resource_slot;
static void *resource_device;
static const void *resource_description;
static const void *resource_initial;
static HRESULT resource_result;
static UINT last_packing_a,last_packing_b;
static void *last_placement;
static void *last_srv_resource;
HRESULT WINAPI FM5CreateLegacyShaderResourceView(void *device,void *resource,const void *description,void **out) {
    resource_device=device;last_srv_resource=resource;resource_description=description;
    if(out)*out=SUCCEEDED(resource_result)?(void *)0x12345678:NULL;
    return resource_result;
}
HRESULT WINAPI FM5CreateLegacyPlacementTexture2D(void *device,const void *description,UINT a,UINT b,void *placement,void **out) {
    resource_device=device;resource_description=description;last_packing_a=a;last_packing_b=b;last_placement=placement;
    if(out)*out=SUCCEEDED(resource_result)?(void *)0x12345678:NULL;
    return resource_result;
}
HRESULT WINAPI FM5CreateLegacyPlacementTexture3D(void *device,const void *description,UINT a,UINT b,void *placement,void **out) {
    resource_device=device;resource_description=description;last_packing_a=a;last_packing_b=b;last_placement=placement;resource_slot=61;
    if(out)*out=SUCCEEDED(resource_result)?(void *)0x12345678:NULL;
    return resource_result;
}
HRESULT WINAPI FM5CreateLegacyPlacementBuffer(void *device,const void *description,void *placement,void **out) {
    resource_device=device;resource_description=description;last_placement=placement;
    if(out)*out=SUCCEEDED(resource_result)?(void *)0x12345678:NULL;
    return resource_result;
}
HRESULT WINAPI FM5CreateLegacyResource(void *device,UINT slot,const void *description,const void *initial,void **out) {
    resource_device=device;resource_slot=slot;resource_description=description;resource_initial=initial;
    if (out) *out=SUCCEEDED(resource_result) ? (void *)0x12345678 : NULL;
    return resource_result;
}
HRESULT WINAPI FM5WrapPerformanceContext(void *owned,void **out) {
    ++wrap_calls; *out=NULL;
    if (owned!=&context) return E_INVALIDARG;
    if (wrap_fail) return E_OUTOFMEMORY;
    *out=owned; return S_OK;
}
static void WINAPI native_get_context(void *self,void **out) { (void)self;na(&context);*out=&context; }
static HRESULT WINAPI native_deferred(void *self,UINT flags,void **out) {
    (void)self;last_flags=flags;na(&context);*out=&context;return native_context_fail ? E_FAIL : S_OK;
}
typedef double (WINAPI *MixedFn)(void *,UINT64,double,float,UINT64,double,UINT64);
typedef UINT64 (WINAPI *IntegerFn)(void *,UINT64,UINT64,UINT64,UINT64,UINT64,UINT64);
typedef union TestCall { void *address;Unknown opaque;MixedFn mixed;IntegerFn integers;
    void (WINAPI *get)(void *,void **);HRESULT (WINAPI *deferred)(void *,UINT,void **);
    HRESULT (WINAPI *resource)(void *,const void *,const void *,void **);
    HRESULT (WINAPI *placement)(void *,const void *,UINT,UINT,void *,void **);
    HRESULT (WINAPI *srv)(void *,void *,const void *,void **);
    HRESULT (WINAPI *pixel)(void *,const void *,SIZE_T,void *,void **);
    BOOL (WINAPI *fence)(void *,UINT64); } TestCall;
static Native *expected;
static BOOL forwarded;
static DWORD fence_tls_index;
static UINT64 last_fence;
static void *last_fence_device;
static BOOL fence_result;
static LONG fence_calls;
static struct {UINT64 fence;void *device;} fence_tls_record;
static BOOL WINAPI native_fence(void *self,UINT64 fence) {
    ++fence_calls;last_fence_device=self;last_fence=fence;
    fence_tls_record.fence=fence;fence_tls_record.device=self;
    TlsSetValue(fence_tls_index,fence_result?&fence_tls_record:NULL);
    return fence_result;
}
static volatile LONG cache_failures;
static DWORD WINAPI cache_worker(void *value) {
    Native *native=(Native *)value;
    for (unsigned i=0;i<500;++i) {
        na(native);void *proxy=NULL;HRESULT hr=FM5WrapPerformanceDevice(native,&proxy);
        if (FAILED(hr) || !proxy) { nr(native);InterlockedIncrement(&cache_failures); }
        else {
            if (FM5UnwrapPerformanceDevice(proxy)!=native) InterlockedIncrement(&cache_failures);
            (*(DeviceVT **)proxy)->release(proxy);
        }
    }
    return 0;
}
static double WINAPI mixed(void *self,UINT64 a,double b,float c,UINT64 d,double e,UINT64 f) {
    forwarded=self==expected&&a==0x1122334455667788&&b==2.25&&c==3.5f&&d==0x8877665544332211&&e==5.75&&f==99;
    return 219.75;
}
static UINT64 WINAPI integers(void *self,UINT64 a,UINT64 b,UINT64 c,UINT64 d,UINT64 e,UINT64 f) {
    forwarded=self==expected&&a==1&&b==2&&c==3&&d==4&&e==5&&f==6;
    return 0x1020304050607080;
}
static const void *pixel_code;
static HRESULT WINAPI native_pixel(void *self,const void *code,SIZE_T bytes,void *linkage,void **out) {
    forwarded=self==expected && code==pixel_code && bytes==(SIZE_T)0xfedcba9876543210 &&
        linkage==(void *)(uintptr_t)0x8877665544332211;
    if(out)*out=SUCCEEDED(resource_result)?(void *)0x12345678:NULL;
    return resource_result;
}
static unsigned checks;
#define CHECK(x) do { ++checks; if (!(x)) { printf("failed_line=%u\n",__LINE__);return 1; } } while (0)
int main(void) {
    Native native={&nvt,1,0,0}; void *out=(void *)1;
    fence_tls_index=TlsAlloc();CHECK(fence_tls_index!=TLS_OUT_OF_INDEXES);
    context.vt=&nvt;context.refs=1;expected=&native;
    TestCall call;call.mixed=mixed;nvt.methods[9-3]=call.address;
    call.integers=integers;nvt.methods[8-3]=call.address;
    call.fence=native_fence;nvt.methods[60-3]=call.address;
    call.pixel=native_pixel;nvt.methods[15-3]=call.address;
    call.get=native_get_context;
    nvt.methods[41-3]=call.address;nvt.methods[44-3]=call.address;nvt.methods[51-3]=call.address;
    call.deferred=native_deferred;
    nvt.methods[27-3]=call.address;nvt.methods[45-3]=call.address;nvt.methods[52-3]=call.address;
    CHECK(FM5WrapPerformanceDevice(&native,NULL)==E_POINTER && native.refs==1);
    CHECK(FM5WrapPerformanceDevice(NULL,&out)==E_INVALIDARG && out==NULL);
    FM5PerformanceFailAllocation(0);
    CHECK(FM5WrapPerformanceDevice(&native,&out)==E_OUTOFMEMORY && out==NULL && native.refs==1);
    CHECK(FM5WrapPerformanceDevice(&native,&out)==S_OK && out!=NULL && native.refs==1);
    void *device=out; DeviceVT *dv=*(DeviceVT **)device;
    CHECK((char *)&dv->feature-(char *)dv==54*8 && (char *)&dv->create-(char *)dv==55*8);
    CHECK(FM5UnwrapPerformanceDevice(device)==&native && FM5UnwrapPerformanceDevice(&native)==&native);
    na(&native); CHECK(FM5WrapPerformanceDevice(&native,&out)==S_OK && out==device && native.refs==1);
    CHECK(dv->release(device)==1 && native.refs==1);
    CHECK(dv->query(device,&iid_unknown,&out)==S_OK && out==device);
    CHECK(dv->release(out)==1);
    CHECK(dv->query(device,&key,&out)==E_NOINTERFACE && !out && native.queries==1);
    CHECK(dv->query(device,&iid_device,&out)==S_OK && out==device && native.refs==1);
    CHECK(dv->release(out)==1);
    dv->add(device);CHECK(FM5WrapPerformanceDevice(device,&out)==S_OK && out==device && dv->release(out)==1);
    FM5ScratchDescription desc={1,65536}, copy=desc;
    CHECK(dv->create(device,&desc,NULL)==E_POINTER);
    CHECK(dv->create(device,NULL,&out)==E_INVALIDARG && !out);
    FM5ScratchDescription wrong={0,65536}; CHECK(dv->create(device,&wrong,&out)==E_INVALIDARG && !out);
    wrong=desc;wrong.bytes=1;CHECK(dv->create(device,&wrong,&out)==E_INVALIDARG && !out);
    FM5PerformanceFailAllocation(0); CHECK(dv->create(device,&desc,&out)==E_OUTOFMEMORY && !out);
    CHECK(dv->create(device,&desc,&out)==S_OK && out && !memcmp(&desc,&copy,sizeof(desc)));
    void *scratch=out; ScratchVT *sv=*(ScratchVT **)scratch;
    CHECK(FM5InspectScratch(scratch,&copy) && !memcmp(&desc,&copy,sizeof(desc)) && sv->get_type(scratch)==2);
    CHECK(!FM5InspectScratch(device,&copy));
    sv->get_device(scratch,&out); CHECK(out==device && dv->release(out)==2);
    CHECK(sv->query(scratch,&iid_unknown,&out)==S_OK && out==scratch && sv->release(out)==1);
    CHECK(sv->query(scratch,&key,&out)==E_NOINTERFACE && !out);
    UINT bytes=9; CHECK(sv->get_private(scratch,&key,&bytes,NULL)==(HRESULT)0x88790002 && bytes==0);
    const char data[]="abcdef";
    CHECK(sv->set_private(scratch,&key,6,data)==S_OK);
    char buffer[8];memset(buffer,'!',sizeof(buffer));bytes=3;
    CHECK(sv->get_private(scratch,&key,&bytes,buffer)==S_OK && bytes==6 && !memcmp(buffer,"abc!!!!!",8));
    bytes=0;CHECK(sv->get_private(scratch,&key,&bytes,NULL)==S_OK && bytes==6);
    FM5PerformanceFailAllocation(0);CHECK(sv->set_private(scratch,&key,3,"bad")==E_OUTOFMEMORY);
    memset(buffer,0,8);bytes=8;CHECK(sv->get_private(scratch,&key,&bytes,buffer)==S_OK && bytes==6 && !memcmp(buffer,data,6));
    FM5PerformanceFailAllocation(1);CHECK(sv->set_private(scratch,&key,3,"bad")==E_OUTOFMEMORY);
    CHECK(sv->set_private(scratch,&key,0,NULL)==S_OK);
    CHECK(sv->set_private(scratch,&key,0,NULL)==(HRESULT)0x88790002);
    Native property={&nvt,1,0,0};
    CHECK(sv->set_interface(scratch,&key,&property)==S_OK && property.refs==2);
    bytes=sizeof(void *);out=NULL;CHECK(sv->get_private(scratch,&key,&bytes,&out)==S_OK && out==&property && property.refs==3);
    nr(out);CHECK(property.refs==2);
    bytes=0;CHECK(sv->get_private(scratch,&key,&bytes,NULL)==S_OK && bytes==8 && property.refs==3);
    nr(&property);
    CHECK(sv->set_interface2(scratch,&key,&property)==S_OK && property.refs==2);
    CHECK(sv->set_interface2(scratch,&key,NULL)==S_OK && property.refs==1);
    CHECK(sv->set_interface(scratch,&key,&property)==S_OK && property.refs==2);
    CHECK(dv->feature(device,4,0)==E_NOTIMPL);
    BOOL rejected=FALSE;
    __try { dv->feature(device,0xff123a00,1); }
    __except(GetExceptionCode()==FM5_E_UNMAPPED_DEVICE_SLOT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { rejected=TRUE; }
    CHECK(rejected);
    rejected=FALSE;
    __try { dv->extended[8](device,(ULONG_PTR)0x11223344,(ULONG_PTR)0x55667788,(ULONG_PTR)0x99aabbcc); }
    __except(GetExceptionCode()==FM5_E_UNMAPPED_DEVICE_SLOT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { rejected=TRUE; }
    CHECK(rejected);
    rejected=FALSE;
    __try { dv->extended[8](device,(ULONG_PTR)0,(ULONG_PTR)0,(ULONG_PTR)0); }
    __except(GetExceptionCode()==FM5_E_UNMAPPED_DEVICE_SLOT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { rejected=TRUE; }
    CHECK(rejected);
    for (UINT slot=3;slot<=6;++slot) {
        call.opaque=dv->unused[slot-3];resource_result=S_OK;out=NULL;
        CHECK(call.resource(device,&desc,&copy,&out)==S_OK && out==(void *)0x12345678 && resource_device==&native && resource_slot==slot && resource_description==&desc && resource_initial==&copy);
        resource_result=E_OUTOFMEMORY;
        CHECK(call.resource(device,&desc,&copy,&out)==E_OUTOFMEMORY && out==NULL);
    }
    call.opaque=dv->extended[60-56];resource_result=S_OK;out=NULL;
    CHECK(call.placement(device,&desc,0x11223344,0x55667788,&copy,&out)==S_OK && out==(void *)0x12345678 && resource_device==&native && resource_description==&desc && last_packing_a==0x11223344 && last_packing_b==0x55667788 && last_placement==&copy);
    resource_result=E_FAIL;CHECK(call.placement(device,&desc,1,2,NULL,&out)==E_FAIL && out==NULL);
    resource_result=E_INVALIDARG;CHECK(call.placement(device,&desc,1,2,NULL,NULL)==E_INVALIDARG);
    call.opaque=dv->extended[61-56];resource_result=S_OK;out=NULL;
    CHECK(call.placement(device,&desc,13,0,&copy,&out)==S_OK && out==(void *)0x12345678 && resource_device==&native && resource_slot==61 && resource_description==&desc && last_packing_a==13 && last_packing_b==0 && last_placement==&copy);
    resource_result=E_FAIL;CHECK(call.placement(device,&desc,0x123456ab,0x00cdef01,NULL,&out)==E_FAIL && out==NULL && last_packing_a==0x123456ab && last_packing_b==0x00cdef01 && !last_placement);
    resource_result=E_INVALIDARG;CHECK(call.placement(device,&desc,1,2,NULL,NULL)==E_INVALIDARG);
    call.opaque=dv->extended[58-56];resource_result=S_OK;out=NULL;
    CHECK(call.resource(device,&desc,&copy,&out)==S_OK && out==(void *)0x12345678 && resource_device==&native && resource_description==&desc && last_placement==&copy);
    resource_result=E_FAIL;CHECK(call.resource(device,&desc,NULL,&out)==E_FAIL && out==NULL);
    resource_result=E_INVALIDARG;CHECK(call.resource(device,&desc,NULL,NULL)==E_INVALIDARG);
    call.opaque=dv->unused[7-3];resource_result=S_OK;out=NULL;
    CHECK(call.srv(device,&copy,&desc,&out)==S_OK && out==(void *)0x12345678 && resource_device==&native && last_srv_resource==&copy && resource_description==&desc);
    resource_result=E_FAIL;CHECK(call.srv(device,NULL,NULL,&out)==E_FAIL && out==NULL);
    resource_result=S_FALSE;CHECK(call.srv(device,&copy,&desc,NULL)==S_FALSE);
    call.opaque=dv->unused[15-3];pixel_code=&copy;resource_result=S_OK;forwarded=FALSE;
    CHECK(call.pixel(device,pixel_code,(SIZE_T)0xfedcba9876543210,(void *)(uintptr_t)0x8877665544332211,&out)==S_OK && forwarded && out==(void *)0x12345678);
    resource_result=E_FAIL;forwarded=FALSE;
    CHECK(call.pixel(device,pixel_code,(SIZE_T)0xfedcba9876543210,(void *)(uintptr_t)0x8877665544332211,&out)==E_FAIL && forwarded && out==NULL);
    resource_result=S_FALSE;forwarded=FALSE;
    CHECK(call.pixel(device,pixel_code,(SIZE_T)0xfedcba9876543210,(void *)(uintptr_t)0x8877665544332211,NULL)==S_FALSE && forwarded);
    UINT64 fences[]={0,1,0xffffffff,0x100000000,0xffffffffffff,0x1000000000000,0x8000000000000000,0xffffffffffffffff};
    call.opaque=dv->extended[56-56];
    for(UINT i=0;i<sizeof(fences)/sizeof(fences[0]);++i) for(UINT busy=0;busy<2;++busy) {
        fence_result=busy!=0;
        CHECK(call.fence(device,fences[i])==fence_result && last_fence==fences[i] && last_fence_device==&native);
        CHECK(TlsGetValue(fence_tls_index)==(fence_result?&fence_tls_record:NULL) && fence_tls_record.fence==fences[i] && fence_tls_record.device==&native);
    }
    CHECK(fence_calls==16);
    void *fence_method=nvt.methods[60-3];nvt.methods[60-3]=NULL;rejected=FALSE;
    __try {call.fence(device,0xdeadbeef12345678);}
    __except(GetExceptionCode()==FM5_E_UNMAPPED_DEVICE_SLOT?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){rejected=TRUE;}
    CHECK(rejected && fence_calls==16 && TlsGetValue(fence_tls_index)==&fence_tls_record);
    nvt.methods[60-3]=fence_method;rejected=FALSE;
    __try {dv->extended[57-56](device,(ULONG_PTR)0x4444,(ULONG_PTR)0,(ULONG_PTR)0);}
    __except(GetExceptionCode()==FM5_E_UNMAPPED_DEVICE_SLOT?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){rejected=TRUE;}
    CHECK(rejected && fence_calls==16);
    call.opaque=dv->unused[9-3];forwarded=FALSE;
    CHECK(call.mixed(device,0x1122334455667788,2.25,3.5f,0x8877665544332211,5.75,99)==219.75 && forwarded);
    call.opaque=dv->unused[8-3];forwarded=FALSE;
    CHECK(call.integers(device,1,2,3,4,5,6)==0x1020304050607080 && forwarded);
    for (unsigned i=0;i<3;++i) {
        unsigned slots[3]={41,44,51};call.opaque=dv->unused[slots[i]-3];out=NULL;
        call.get(device,&out);CHECK(out==&context && context.refs==2);nr(out);
    }
    for (unsigned i=0;i<3;++i) {
        unsigned slots[3]={27,45,52};call.opaque=dv->unused[slots[i]-3];out=NULL;
        CHECK(call.deferred(device,0x10203040,&out)==S_OK && out==&context && context.refs==2 && last_flags==0x10203040);nr(out);
    }
    wrap_fail=TRUE;call.opaque=dv->unused[27-3];
    CHECK(call.deferred(device,0,&out)==E_OUTOFMEMORY && out==NULL && context.refs==1);
    wrap_fail=FALSE;native_context_fail=TRUE;
    CHECK(call.deferred(device,0,&out)==E_FAIL && out==NULL && context.refs==1);
    native_context_fail=FALSE;wrap_fail=TRUE;rejected=FALSE;call.opaque=dv->unused[41-3];
    __try { call.get(device,&out); }
    __except(GetExceptionCode()==FM5_E_UNMAPPED_DEVICE_SLOT ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { rejected=TRUE; }
    CHECK(rejected && out==NULL && context.refs==1);
    wrap_fail=FALSE;
    HANDLE threads[4];
    for (unsigned i=0;i<4;++i) { threads[i]=CreateThread(NULL,0,cache_worker,&native,0,NULL);CHECK(threads[i]!=NULL); }
    CHECK(WaitForMultipleObjects(4,threads,TRUE,10000)==WAIT_OBJECT_0);
    for (unsigned i=0;i<4;++i) CloseHandle(threads[i]);
    CHECK(cache_failures==0 && native.refs==1);
    CHECK(sv->release(scratch)==0 && property.refs==1 && native.refs==1);
    CHECK(dv->release(device)==0 && native.refs==0);
    CHECK(FM5UnwrapPerformanceDevice(device)==device);
    CHECK(TlsFree(fence_tls_index));
    printf("{\"passed\":true,\"checks\":%u,\"concurrent_cache_operations\":2000,\"gpu_allocations\":0,\"context_wrap_calls\":%ld,\"fence_calls\":%ld,\"fence_tls_preserved\":true,\"scope\":\"native descriptor, COM lifetime, property bag, static ABI tailcalls, context ownership\"}\n",checks,wrap_calls,fence_calls);
    return 0;
}
