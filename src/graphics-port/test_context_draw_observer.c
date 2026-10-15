#define CINTERFACE
#pragma warning(push)
#pragma warning(disable:4201)
#include <d3d11_1.h>
#pragma warning(pop)
#include <stdio.h>
#include <assert.h>
#include "fm5_performance_context.h"
typedef struct Native {void **table;ULONG refs;} Native;
static Native native;
static UINT draws,observations,maps,unmaps,map_observations,unmap_observations;
static UINT order;
static HRESULT map_status;
static D3D11_MAPPED_SUBRESOURCE output;
static ULONG release(void*self){assert(self==&native);return --native.refs;}
static void draw(void*self,UINT count,UINT start){assert(self==&native&&count==0xfedcba98&&start==0x12345678);++draws;}
static void indexed(void*self,UINT64 packed,INT base){assert(self==&native&&packed==0xfedcba9812345678ull&&base==-123);++draws;}
static void indexed_instanced(void*self,UINT64 packed,UINT64 vertices,UINT instances){
 assert(self==&native&&packed==0xfedcba9812345678ull&&vertices==0xaabbccddffffff85ull&&instances==0xf00d);++draws;
}
static void instanced(void*self,UINT count,UINT64 vertices,UINT instances){
 assert(self==&native&&count==0xfedcba98&&vertices==0xaabbccdd12345678ull&&instances==0xf00d);++draws;
}
static HRESULT mapped(void*self,ID3D11Resource *resource,UINT sub,D3D11_MAP type,UINT flags,D3D11_MAPPED_SUBRESOURCE *out){
 assert(self==&native&&resource==(void*)(UINT_PTR)0x98765000&&sub==0xffffffff&&type==D3D11_MAP_WRITE_DISCARD&&flags==0x88776655&&out==&output);
 order=1;++maps;out->pData=(void*)0x1234567887654321ull;out->RowPitch=0xaabbccdd;out->DepthPitch=0xffffffff;return map_status;
}
static void unmapped(void*self,ID3D11Resource *resource,UINT sub){
 assert(self==&native&&resource==(void*)(UINT_PTR)0x98765000&&sub==0xffffffff);assert(!unmap_observations||order==3);order=4;++unmaps;
}
static void observe_draw(void*self,BOOL after){assert(self==&native&&observations==(after?draws*2-1:draws*2));++observations;}
static void observe_map(void*self,void*resource,UINT sub,UINT type,HRESULT result,const void*out){
 assert(self==&native&&resource==(void*)(UINT_PTR)0x98765000&&sub==0xffffffff&&type==4&&result==map_status&&out==&output);
 assert(order==1&&output.RowPitch==0xaabbccdd&&output.pData==(void*)0x1234567887654321ull);order=2;++map_observations;
}
static void observe_unmap(void*self,void*resource,UINT sub){
 assert(self==&native&&resource==(void*)(UINT_PTR)0x98765000&&sub==0xffffffff&&order==2);order=3;++unmap_observations;
}
int main(void){
 void *table[270]={0},*proxy=NULL;table[2]=(void*)release;table[13]=(void*)indexed;table[14]=(void*)draw;
 table[21]=(void*)indexed_instanced;table[22]=(void*)instanced;table[15]=(void*)mapped;table[16]=(void*)unmapped;
 native.table=table;native.refs=1;assert(FM5WrapPerformanceContext(&native,&proxy)==S_OK);
 FM5ConfigureContextDrawObserver(observe_draw);FM5ConfigureContextMapObserver(observe_map,observe_unmap);
 ((void(*)(void*,UINT,UINT))(*(void***)proxy)[14])(proxy,0xfedcba98,0x12345678);
 ((void(*)(void*,UINT,UINT,INT))(*(void***)proxy)[13])(proxy,0xfedcba98,0x12345678,-123);
 ((void(*)(void*,UINT,UINT,UINT,INT,UINT))(*(void***)proxy)[21])(proxy,0xfedcba98,0xf00d,0x12345678,-123,0xaabbccdd);
 ((void(*)(void*,UINT,UINT,UINT,UINT))(*(void***)proxy)[22])(proxy,0xfedcba98,0xf00d,0x12345678,0xaabbccdd);
 assert(draws==4&&observations==8);
 const HRESULT results[]={S_OK,S_FALSE,E_FAIL,E_OUTOFMEMORY};
 for(UINT i=0;i<4;++i){map_status=results[i];
  assert(((HRESULT(*)(void*,void*,UINT,UINT,UINT,void*))(*(void***)proxy)[15])
   (proxy,(void*)(UINT_PTR)0x98765000,0xffffffff,4,0x88776655,&output)==map_status);
  ((void(*)(void*,void*,UINT))(*(void***)proxy)[16])(proxy,(void*)(UINT_PTR)0x98765000,0xffffffff);
  assert(order==4);
 }
 assert(maps==4&&unmaps==4&&map_observations==4&&unmap_observations==4);
 FM5ConfigureContextDrawObserver(NULL);FM5ConfigureContextMapObserver(NULL,NULL);unmap_observations=0;
 ((void(*)(void*,UINT,UINT))(*(void***)proxy)[14])(proxy,0xfedcba98,0x12345678);
 assert(draws==5&&observations==8);map_status=S_OK;
 assert(((HRESULT(*)(void*,void*,UINT,UINT,UINT,void*))(*(void***)proxy)[15])
  (proxy,(void*)(UINT_PTR)0x98765000,0xffffffff,4,0x88776655,&output)==S_OK);
 ((void(*)(void*,void*,UINT))(*(void***)proxy)[16])(proxy,(void*)(UINT_PTR)0x98765000,0xffffffff);
 assert(maps==5&&unmaps==5&&map_observations==4);
 assert(((ULONG(*)(void*))(*(void***)proxy)[2])(proxy)==0&&native.refs==0);
 puts("PASS: four authentic Draw ABIs, packed arguments, six-argument Map results/output, before-Unmap order, callback-disabled forwarding and lifetime");return 0;
}
