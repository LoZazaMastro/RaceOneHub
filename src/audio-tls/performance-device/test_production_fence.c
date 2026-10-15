#include "fm5_performance_device.h"
#include <stdio.h>
typedef ULONG (WINAPI *RefFn)(void *);
typedef BOOL (WINAPI *FenceFn)(void *,UINT64);
typedef union Call {void *pointer;RefFn ref;FenceFn fence;} Call;
typedef struct Fake {void **table;ULONG refs;} Fake;
static LONG backend_calls;
static ULONG WINAPI add(void *self){return ++((Fake *)self)->refs;}
static ULONG WINAPI release(void *self){return --((Fake *)self)->refs;}
static BOOL WINAPI fence(void *self,UINT64 value){(void)self;(void)value;++backend_calls;return TRUE;}
HRESULT WINAPI FM5WrapPerformanceContext(void *native,void **out){(void)native;(void)out;return E_NOTIMPL;}
HRESULT WINAPI FM5CreateLegacyResource(void *native,UINT slot,const void *desc,const void *initial,void **out){(void)native;(void)slot;(void)desc;(void)initial;(void)out;return E_NOTIMPL;}
HRESULT WINAPI FM5CreateLegacyPlacementBuffer(void *native,const void *desc,void *placement,void **out){(void)native;(void)desc;(void)placement;(void)out;return E_NOTIMPL;}
HRESULT WINAPI FM5CreateLegacyPlacementTexture2D(void *native,const void *desc,UINT a,UINT b,void *placement,void **out){(void)native;(void)desc;(void)a;(void)b;(void)placement;(void)out;return E_NOTIMPL;}
HRESULT WINAPI FM5CreateLegacyPlacementTexture3D(void *native,const void *desc,UINT a,UINT b,void *placement,void **out){(void)native;(void)desc;(void)a;(void)b;(void)placement;(void)out;return E_NOTIMPL;}
HRESULT WINAPI FM5CreateLegacyShaderResourceView(void *native,void *resource,const void *desc,void **out){(void)native;(void)resource;(void)desc;(void)out;return E_NOTIMPL;}
int main(void){
    void *table[65]={0};Call call;call.ref=add;table[1]=call.pointer;call.ref=release;table[2]=call.pointer;call.fence=fence;table[60]=call.pointer;
    Fake native={table,1};void *proxy=NULL;
    if(FAILED(FM5WrapPerformanceDevice(&native,&proxy)))return 1;
    call.pointer=(*(void ***)proxy)[56];BOOL rejected=FALSE;
    __try {call.fence(proxy,0x1122334455667788);}
    __except(GetExceptionCode()==FM5_E_UNMAPPED_DEVICE_SLOT?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){rejected=TRUE;}
    call.pointer=(*(void ***)proxy)[2];call.ref(proxy);
    BOOL passed=rejected && !backend_calls && !native.refs;
    printf("{\"passed\":%s,\"production_guard\":true,\"backend_calls\":%ld,\"native_refs\":%lu}\n",passed?"true":"false",backend_calls,native.refs);
    return passed?0:1;
}
