#define CINTERFACE
#include <d3d11_1.h>
#include <assert.h>
#include <stdio.h>
#include "fm5_performance_context.h"
#include "../audio-tls/performance-device/fm5_performance_device.h"
typedef struct Native {void **table;ULONG refs;} Native;
static Native device, context;
static unsigned packed_calls, topology_calls;
static UINT expected_index_format;
static void *expected_index_buffer;
HRESULT WINAPI FM5CreateLegacyPlacementBuffer(void*n,const void*d,void*p,void**out) {
    (void)n;(void)d;(void)p;(void)out;assert(0);return E_NOTIMPL;
}
HRESULT WINAPI FM5CreateLegacyPlacementTexture2D(void*n,const void*d,UINT a,UINT b,void*p,void**out) {
    (void)n;(void)d;(void)a;(void)b;(void)p;(void)out;assert(0);return E_NOTIMPL;
}
HRESULT WINAPI FM5CreateLegacyPlacementTexture3D(void*n,const void*d,UINT a,UINT b,void*p,void**out) {
    (void)n;(void)d;(void)a;(void)b;(void)p;(void)out;assert(0);return E_NOTIMPL;
}
HRESULT WINAPI FM5CreateLegacyShaderResourceView(void*n,void*r,const void*d,void**out) {
    (void)n;(void)r;(void)d;(void)out;assert(0);return E_NOTIMPL;
}
/* This fixture never creates a GPU resource; satisfy the Device component's
 * real production linkage without hiding an accidental resource call. */
HRESULT WINAPI FM5CreateLegacyResource(void *native, UINT slot, const void *description,
                                      const void *initial, void **out) {
    (void)native; (void)slot; (void)description; (void)initial; (void)out;
    assert(0);
    return E_NOTIMPL;
}
static HRESULT nq(void *self,const GUID *iid,void **out){(void)iid;((Native*)self)->refs++;*out=self;return S_OK;}
static ULONG nr(void*self){assert(((Native*)self)->refs);return --((Native*)self)->refs;}
static void gd(void*self,void **out){assert(self==&context);device.refs++;*out=&device;}
static void gc(void*self,void **out){assert(self==&device);context.refs++;*out=&context;}
static HRESULT topo(void*self,UINT value){assert(self==&context&&value==5);++topology_calls;return S_OK;}
static void ib(void*self,UINT fmt,void*buffer,UINT off){assert(self==&context&&fmt==expected_index_format&&buffer==expected_index_buffer&&off==0xffffffffu);++packed_calls;}
static void di(void*self,UINT64 indices,INT base){assert(self==&context&&indices==0xffffffff12345678ull&&base==-19);++packed_calls;}
static void dii(void*self,UINT64 indices,UINT64 vertices,UINT instances){assert(self==&context&&indices==0xffffffff12345678ull&&vertices==0x87654321ffffffedull&&instances==0xaabbccddu);++packed_calls;}
static void din(void*self,UINT count,UINT64 vertices,UINT instances){assert(self==&context&&count==0xffffffffu&&vertices==0x8765432112345678ull&&instances==0xaabbccddu);++packed_calls;}
static ULONG rel(void*x){return((ULONG(*)(void*))(*(void***)x)[2])(x);}
int main(void){
void *dt[270]={0},*ct[270]={0},*dp,*cp,*other,*queried;GUID normal={0xc0bfa96c,0xe089,0x44fb,{0x8e,0xaf,0x26,0xf8,0x79,0x61,0x90,0xda}};
device.table=dt;device.refs=1;context.table=ct;context.refs=1;dt[0]=ct[0]=(void*)nq;dt[2]=ct[2]=(void*)nr;dt[41]=(void*)gc;ct[3]=(void*)gd;
ct[0x14]=(void*)ib;ct[0x0d]=(void*)di;ct[0x15]=(void*)dii;ct[0x16]=(void*)din;
FM5ConfigureContextInterop(topo,FM5WrapPerformanceDevice);
assert(FM5WrapPerformanceDevice(&device,&dp)==S_OK);assert(FM5WrapPerformanceContext(&context,&cp)==S_OK);
assert(FM5UnwrapPerformanceContext(cp)==&context&&FM5UnwrapPerformanceContext(&context)==&context);
((void(*)(void*,void**))(*(void***)cp)[3])(cp,&other);assert(other==dp&&device.refs==1);assert(rel(other)==1);
((void(*)(void*,void**))(*(void***)dp)[41])(dp,&other);assert(other==cp&&context.refs==1);assert(rel(other)==1);
assert(((HRESULT(*)(void*,const GUID*,void**))(*(void***)cp)[0])(cp,&normal,&queried)==S_OK);assert(queried==cp&&context.refs==1);assert(rel(queried)==1);
expected_index_buffer=(void*)(UINT_PTR)0x1234567887654321;expected_index_format=1;
((void(*)(void*,void*,UINT,UINT))(*(void***)cp)[0x14])(cp,expected_index_buffer,DXGI_FORMAT_R32_UINT,0xffffffffu);
expected_index_format=0;
((void(*)(void*,void*,UINT,UINT))(*(void***)cp)[0x14])(cp,expected_index_buffer,DXGI_FORMAT_R16_UINT,0xffffffffu);
expected_index_buffer=NULL;
((void(*)(void*,void*,UINT,UINT))(*(void***)cp)[0x14])(cp,NULL,DXGI_FORMAT_UNKNOWN,0xffffffffu);
((void(*)(void*,void*,UINT,UINT))(*(void***)cp)[0x14])(cp,NULL,DXGI_FORMAT_R32_UINT,0xffffffffu);
unsigned before_invalid=packed_calls;
BOOL rejected=FALSE;
__try {
    ((void(*)(void*,void*,UINT,UINT))(*(void***)cp)[0x14])(cp,(void*)0x12340,DXGI_FORMAT_R8_UINT,0xffffffffu);
} __except(GetExceptionCode()==0xe0f50006?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {
    rejected=TRUE;
}
assert(rejected&&packed_calls==before_invalid);
((void(*)(void*,UINT,UINT,INT))(*(void***)cp)[0x0d])(cp,0xffffffffu,0x12345678,-19);
((void(*)(void*,UINT,UINT,UINT,INT,UINT))(*(void***)cp)[0x15])(cp,0xffffffffu,0xaabbccdd,0x12345678,-19,0x87654321);
((void(*)(void*,UINT,UINT,UINT,UINT))(*(void***)cp)[0x16])(cp,0xffffffffu,0xaabbccdd,0x12345678,0x87654321);
((void(*)(void*,UINT))(*(void***)cp)[0x19])(cp,5);
assert(packed_calls==7&&topology_calls==1);assert(rel(cp)==0&&context.refs==0);assert(rel(dp)==0&&device.refs==0);
puts("PASS: R16/R32 Xbox index enums, NULL unbind, invalid index rejection, full-width buffer/offset, three packed draws, topology and Context-Device identity cleanup");return 0;}
