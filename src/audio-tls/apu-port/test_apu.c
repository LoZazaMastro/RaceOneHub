#include "fm5_apu.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>
static unsigned checks,create_calls,alloc_calls,shape_calls,hal_calls;
static BYTE ready;
static FM5APUHeap heaps[2];
static SIZE_T used[2];
static int trees[2],hal_object;
static HRESULT create_status=S_OK,alloc_status=S_OK,shape_status=S_OK,hal_status=S_OK;
static BOOL incomplete;
static UINT last_size,last_alignment,last_type;
static void *last_shape;
#define CHECK(x) do{++checks;if(!(x)){printf("failed_line=%u\n",__LINE__);return 1;}}while(0)
static void cleanup(void){for(UINT i=0;i<2;++i)if(heaps[i].cpu)VirtualFree(heaps[i].cpu,0,MEM_RELEASE);memset(heaps,0,sizeof(heaps));memset(used,0,sizeof(used));ready=0;}
static HRESULT WINAPI create_heap(UINT32 first,UINT32 second){
 ++create_calls;if(first!=FM5_APU_HEAP1_BYTES || second!=FM5_APU_HEAP2_BYTES)return E_INVALIDARG;
 if(FAILED(create_status)||incomplete)return create_status;
 Sleep(10);
 void *a=VirtualAlloc(NULL,first,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!a)return E_OUTOFMEMORY;
 void *b=VirtualAlloc(NULL,second,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);if(!b){VirtualFree(a,0,MEM_RELEASE);return E_OUTOFMEMORY;}
 heaps[0].cpu=a;heaps[0].bytes=first;heaps[0].apu=FM5_APU_HEAP1_ADDRESS;heaps[0].tree=&trees[0];
 heaps[1].cpu=b;heaps[1].bytes=second;heaps[1].apu=FM5_APU_HEAP2_ADDRESS;heaps[1].tree=&trees[1];ready=1;return create_status;
}
static HRESULT WINAPI allocate(void **cpu,UINT32 *apu,UINT32 bytes,UINT32 alignment,UINT32 type){
 ++alloc_calls;last_size=bytes;last_alignment=alignment;last_type=type;
 if(FAILED(alloc_status))return alloc_status;
 UINT i=type-1;SIZE_T offset=(used[i]+alignment-1)&~((SIZE_T)alignment-1);
 if(offset>heaps[i].bytes || bytes>heaps[i].bytes-offset)return E_OUTOFMEMORY;
 *cpu=(BYTE *)heaps[i].cpu+offset;if(apu)*apu=heaps[i].apu+(UINT32)offset;used[i]=offset+bytes;return alloc_status;
}
static HRESULT WINAPI shape(void *p){++shape_calls;last_shape=p;return shape_status;}
static HRESULT WINAPI hal(void **out){++hal_calls;if(SUCCEEDED(hal_status))*out=&hal_object;return hal_status;}
static DWORD WINAPI worker(void *p){(void)p;return (DWORD)FM5EnsureApuDefaultHeap();}
int main(void){
 FM5APUTestBackends b={create_heap,allocate,shape,hal,&ready,heaps};FM5APUSetTestBackends(&b);
 void *cpu=(void *)1;UINT32 apu=1;BYTE contexts[96];memset(contexts,0x5a,96);
 CHECK(FM5ApuAlloc(&cpu,&apu,1,4,0)==E_INVALIDARG&&!cpu&&!apu&&!create_calls);
 CHECK(FM5ApuAlloc(NULL,&apu,1,4,1)==E_INVALIDARG&&!apu&&!create_calls);
 CHECK(FM5ApuAlloc(&cpu,&apu,0,4,1)==E_INVALIDARG&&!create_calls);
 CHECK(FM5ApuAlloc(&cpu,&apu,1,3,1)==E_INVALIDARG&&!create_calls);
 CHECK(FM5ApuAlloc(&cpu,&apu,1,6,1)==E_INVALIDARG&&!create_calls);
 CHECK(FM5ApuAlloc(&cpu,&apu,1,4,3)==E_INVALIDARG&&!create_calls);
 CHECK(FM5AcpHalCreate(NULL)==E_INVALIDARG&&!create_calls);
 CHECK(FM5AcpHalAllocateShapeContexts(NULL)==E_INVALIDARG&&!create_calls);
 create_status=E_OUTOFMEMORY;
 CHECK(FM5AcpHalCreate(&cpu)==E_OUTOFMEMORY&&!cpu&&create_calls==1&&!hal_calls);
 CHECK(FM5AcpHalAllocateShapeContexts(contexts)==E_OUTOFMEMORY&&create_calls==2&&!shape_calls&&contexts[0]==0x5a);
 CHECK(FM5ApuAlloc(&cpu,&apu,17,4,1)==E_OUTOFMEMORY&&!cpu&&!apu&&create_calls==3&&!alloc_calls);
 create_status=S_OK;incomplete=TRUE;CHECK(FM5EnsureApuDefaultHeap()==E_UNEXPECTED&&!ready);incomplete=FALSE;
 unsigned before=create_calls;HANDLE threads[16];for(UINT i=0;i<16;++i)threads[i]=CreateThread(NULL,0,worker,NULL,0,NULL);
 CHECK(WaitForMultipleObjects(16,threads,TRUE,10000)==WAIT_OBJECT_0);
 UINT success=0,already=0;for(UINT i=0;i<16;++i){DWORD code;CHECK(GetExitCodeThread(threads[i],&code));if(code==S_OK)++success;else if(code==S_FALSE)++already;CloseHandle(threads[i]);}
 CHECK(success==1&&already==15&&create_calls==before+1&&ready);
 CHECK(FM5EnsureApuDefaultHeap()==S_FALSE&&create_calls==before+1);
 CHECK(heaps[0].apu+heaps[0].bytes==heaps[1].apu && (UINT64)heaps[1].apu+heaps[1].bytes<UINT32_MAX);
 CHECK(FM5ApuAlloc(&cpu,&apu,17,4,1)==S_OK&&last_size==17&&last_alignment==4&&last_type==1&&apu==FM5_APU_HEAP1_ADDRESS&&cpu==heaps[0].cpu);
 memset(cpu,0x83,17);CHECK(((BYTE *)heaps[0].cpu)[16]==0x83);
 CHECK(FM5ApuAlloc(&cpu,&apu,0x123,0x10000,2)==S_OK&&last_size==0x123&&last_alignment==0x10000&&last_type==2&&apu==FM5_APU_HEAP2_ADDRESS&&cpu==heaps[1].cpu);
 CHECK(FM5ApuAlloc(&cpu,NULL,0x4000001,4,1)==E_OUTOFMEMORY&&!cpu);
 alloc_status=S_FALSE;CHECK(FM5ApuAlloc(&cpu,&apu,4,4,2)==S_FALSE&&cpu!=NULL);alloc_status=E_FAIL;
 CHECK(FM5ApuAlloc(&cpu,&apu,4,4,2)==E_FAIL&&!cpu&&!apu);alloc_status=S_OK;
 CHECK(FM5AcpHalAllocateShapeContexts(contexts)==S_OK&&last_shape==contexts&&shape_calls==1&&contexts[0]==0x5a);
 shape_status=E_FAIL;CHECK(FM5AcpHalAllocateShapeContexts(contexts)==E_FAIL&&shape_calls==2);
 CHECK(FM5AcpHalCreate(&cpu)==S_OK&&cpu==&hal_object&&hal_calls==1);
 hal_status=E_OUTOFMEMORY;CHECK(FM5AcpHalCreate(&cpu)==E_OUTOFMEMORY&&!cpu&&hal_calls==2);
 heaps[0].bytes--;CHECK(FM5EnsureApuDefaultHeap()==HRESULT_FROM_WIN32(ERROR_INVALID_DATA));heaps[0].bytes++;
 cleanup();FM5APUSetTestBackends(NULL);CHECK(FM5EnsureApuDefaultHeap()==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT));
 printf("{\"passed\":true,\"checks\":%u,\"create_calls\":%u,\"alloc_calls\":%u,\"shape_calls\":%u,\"hal_calls\":%u,\"concurrent_once_threads\":16}\n",checks,create_calls,alloc_calls,shape_calls,hal_calls);return 0;
}
