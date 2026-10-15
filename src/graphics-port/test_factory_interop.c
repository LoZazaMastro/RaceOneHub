#define CINTERFACE
#include <d3d11_1.h>
#include <assert.h>
#include <stdio.h>
#include "fm5_graphics_bridge.h"
#include "fm5_performance_context.h"
#include "../audio-tls/performance-device/fm5_performance_device.h"
typedef struct Native{void**table;LONG refs;}Native;static Native device,context;static unsigned completion,topology,copy_calls,description_calls;
HRESULT WINAPI FM5CreateLegacyPlacementTexture2D(void*n,const void*d,UINT a,UINT b,void*p,void**out){(void)n;(void)d;(void)a;(void)b;(void)p;(void)out;assert(0);return E_NOTIMPL;}
HRESULT WINAPI FM5CreateLegacyPlacementTexture3D(void*n,const void*d,UINT a,UINT b,void*p,void**out){(void)n;(void)d;(void)a;(void)b;(void)p;(void)out;assert(0);return E_NOTIMPL;}
HRESULT WINAPI FM5CreateLegacyPlacementBuffer(void*n,const void*d,void*p,void**out){(void)n;(void)d;(void)p;(void)out;assert(0);return E_NOTIMPL;}
HRESULT WINAPI FM5CreateLegacyShaderResourceView(void*n,void*r,const void*d,void**out){(void)n;(void)r;(void)d;(void)out;assert(0);return E_NOTIMPL;}
HRESULT WINAPI FM5GetLegacyTexture2DDescription(void*r,void*d){assert(r==(void*)0x12340||r==(void*)0x56780);ZeroMemory(d,48);((UINT*)d)[5]=1;description_calls++;return S_OK;}
static void copy(void*self,void*dst,UINT di,UINT x,UINT y,UINT z,void*src,UINT si,const void*box){assert(self==&context&&dst==(void*)0x56780&&src==(void*)0x12340&&di==5&&si==7&&!x&&!y&&!z&&!box);copy_calls++;}
HRESULT WINAPI FM5ReadNativeScissorSnapshot(void*n,UINT*c,void*r){(void)n;(void)c;(void)r;assert(0);return E_NOTIMPL;}
HRESULT WINAPI FM5GetNativePixelSamplers(void*n,UINT s,UINT c,void**r){(void)n;(void)s;(void)c;(void)r;assert(0);return E_NOTIMPL;}
static HRESULT nq(void*self,const GUID*g,void**out){(void)g;((Native*)self)->refs++;*out=self;return S_OK;}
static ULONG nr(void*self){assert(((Native*)self)->refs>0);return --((Native*)self)->refs;}
static void gd(void*self,void**out){assert(self==&context);device.refs++;*out=&device;}
static void gc(void*self,void**out){assert(self==&device);context.refs++;*out=&context;}
HRESULT FM5ApplyNativeCompletionPolicy(void*self,BOOL enabled){assert(self==&context&&enabled==1);completion++;return S_OK;}
HRESULT FM5ApplyNativeTopology(void*self,UINT value){assert(self==&context&&value==4);topology++;return S_OK;}
HRESULT WINAPI FM5CreateLegacyResource(void*n,UINT slot,const void*d,const void*i,void**out){(void)n;(void)slot;(void)d;(void)i;(void)out;assert(0);return E_NOTIMPL;}
static FM5HRESULT factory(const FM5CreateDescriptor*d,void**dev,void**ctx){assert(d->version==0x1000b);device.refs=1;context.refs=1;*dev=&device;*ctx=&context;return S_OK;}
static ULONG rel(void*p){return ((ULONG(*)(void*))(*(void***)p)[2])(p);}
int main(void){void*dt[270]={0},*ct[270]={0},*dev,*ctx,*same;FM5UINT level=0xb100;device.table=dt;context.table=ct;dt[0]=ct[0]=(void*)nq;dt[2]=ct[2]=(void*)nr;dt[41]=(void*)gc;ct[3]=(void*)gd;ct[0x2e]=(void*)copy;
assert(FM5ConfigureGraphicsBridge(factory,1)==S_OK);assert(FM5D3D11CreateDevice(NULL,1,NULL,0x20,&level,1,0x10002,&dev,NULL,&ctx)==S_OK);assert(dev!=&device&&ctx!=&context&&device.refs==1&&context.refs==1);
((void(*)(void*,void**))(*(void***)dev)[41])(dev,&same);assert(same==ctx);assert(rel(same)==1);
((void(*)(void*,void**))(*(void***)ctx)[3])(ctx,&same);assert(same==dev);assert(rel(same)==1);
((void(*)(void*,BOOL))(*(void***)ctx)[0xbf])(ctx,TRUE);((void(*)(void*,UINT))(*(void***)ctx)[0x19])(ctx,4);assert(completion==1&&topology==1);
((void(*)(void*,void*,UINT,void*,UINT,UINT))(*(void***)ctx)[0x3a])(ctx,(void*)0x56780,5,(void*)0x12340,7,0xfeedabcd);assert(copy_calls==1&&description_calls==2);
assert(rel(ctx)==0&&rel(dev)==0&&device.refs==0&&context.refs==0);
FM5PerformanceFailAllocation(0);assert(FM5D3D11CreateDevice(NULL,1,NULL,0x20,&level,1,0x10002,&dev,NULL,&ctx)==E_OUTOFMEMORY);assert(!dev&&!ctx&&device.refs==0&&context.refs==0);
puts("PASS: production interop factory output wrapping, configured topology/completion/legacy-resolve callbacks, context-device identity, native reference adoption, allocation-failure rollback");return 0;}
