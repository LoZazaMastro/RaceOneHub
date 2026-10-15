#define WIN32_LEAN_AND_MEAN
#include "fm5_luminance_readback.h"
#pragma warning(push)
#pragma warning(disable:4201)
#include <d3d12.h>
#pragma warning(pop)
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

namespace {
enum Phase {Idle,Building,Recorded,Submitted,Quarantined};
struct Job {
 Phase phase;UINT64 serial;LONGLONG requested;DWORD deadline;
 void *native_view,*native_resource,*list;
 ID3D12Resource *physical,*readback;ID3D12Fence *fence;
 ID3D12CommandQueue *queue;ID3D12Device *device;
 UINT original_state;D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint;
};
static SRWLOCK lock=SRWLOCK_INIT;
static Job job={};static UINT64 serial;static LONGLONG frequency,last_request;
static volatile LONG active;
static LONGLONG last_refusal;static UINT refusals;
static const BYTE *host;static BOOL enabled;static UINT captures;
#ifdef FM5_LUMINANCE_READBACK_TEST
static UINT last_result;static BOOL result_valid;
#endif
typedef void (*Barriers)(void*,UINT,const D3D12_RESOURCE_BARRIER*);
typedef void (*Copy)(void*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*);
static Barriers barriers;static Copy copy;
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
struct Guard{UINT rva;BYTE bytes[16];};
static const Guard guards[]={
 {0xee080,{0x40,0x56,0x57,0x48,0x83,0xec,0x38,0x48,0x8b,0xc1,0x8b,0xfa,0x48,0x8b,0x09,0x49}},
 {0xee420,{0x40,0x53,0x48,0x81,0xec,0x60,0x01,0x00,0x00,0x48,0x8b,0x9c,0x24,0x98,0x01,0x00}},
 {0xe0cd0,{0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57}},
 {0xe84e0,{0x48,0x89,0x5c,0x24,0x10,0x57,0x48,0x83,0xec,0x60,0x48,0x8b,0x05,0x4f,0xfd,0x22}},
};
static BOOL read(const void *base,SIZE_T offset,void *out,SIZE_T size){
 uintptr_t address=(uintptr_t)base;if(!address || offset>UINTPTR_MAX-address || size>UINTPTR_MAX-address-offset)return FALSE;
 SIZE_T count=0;return ReadProcessMemory(GetCurrentProcess(),(const void*)(address+offset),out,size,&count)&&count==size;
}
static void *pointer(const void *base,SIZE_T offset){void *value=NULL;read(base,offset,&value,8);return value;}
static UINT word(const void *base,SIZE_T offset){UINT value=UINT_MAX;read(base,offset,&value,4);return value;}
static void native_addref(void *p){if(p)((IUnknown*)p)->AddRef();}
static void native_release(void *p){if(p)((IUnknown*)p)->Release();}
static void clean(Job *j){
 if(j->readback)j->readback->Release();if(j->fence)j->fence->Release();
 if(j->physical)j->physical->Release();if(j->queue)j->queue->Release();if(j->device)j->device->Release();
 native_release(j->native_view);native_release(j->native_resource);ZeroMemory(j,sizeof(*j));
}
static void log_event(const Job *j,const char *status,HRESULT hr,const UINT *bits=NULL){
 LARGE_INTEGER tick;QueryPerformanceCounter(&tick);
 float value=0;if(bits)memcpy(&value,bits,4);
 fprintf(stderr,"{\"event\":\"fm5_luminance_readback\",\"serial\":%llu,\"status\":\"%s\",\"hr\":\"0x%08lx\",\"qpc\":%lld,\"requested_qpc\":%lld,\"qpc_frequency\":%lld,\"view\":\"%p\",\"resource\":\"%p\",\"physical\":\"%p\",\"list\":\"%p\",\"queue\":\"%p\",\"state\":%u,\"float_bits\":\"%08x\",\"float_finite\":%s,\"value\":%.9g}\n",
 (unsigned long long)j->serial,status,(unsigned long)hr,tick.QuadPart,j->requested,frequency,j->native_view,j->native_resource,j->physical,j->list,j->queue,j->original_state,bits?*bits:0,bits&&isfinite(value)?"true":"false",bits&&isfinite(value)?(double)value:0.0);fflush(stderr);
}
static BOOL CALLBACK initialize(PINIT_ONCE,void*,void**){
 WCHAR option[2]={};if(GetEnvironmentVariableW(L"FM5_LUMINANCE_READBACK",option,2)!=1 || option[0]!=L'1')return TRUE;
 host=(const BYTE*)GetModuleHandleW(NULL);LARGE_INTEGER f;
 if(!host || !QueryPerformanceFrequency(&f)){fprintf(stderr,"{\"event\":\"fm5_luminance_readback_init\",\"status\":\"host_or_clock_unavailable\"}\n");fflush(stderr);return TRUE;}
 const IMAGE_DOS_HEADER *dos=(const IMAGE_DOS_HEADER*)host;
 if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0x40 || dos->e_lfanew>0x1000){fprintf(stderr,"{\"event\":\"fm5_luminance_readback_init\",\"status\":\"invalid_dos_header\"}\n");fflush(stderr);return TRUE;}
 const IMAGE_NT_HEADERS64 *pe=(const IMAGE_NT_HEADERS64*)(host+dos->e_lfanew);
 if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || pe->OptionalHeader.SizeOfImage<0x12ae20){fprintf(stderr,"{\"event\":\"fm5_luminance_readback_init\",\"status\":\"invalid_pe_header\"}\n");fflush(stderr);return TRUE;}
 for(UINT i=0;i<_countof(guards);++i)if(memcmp(host+guards[i].rva,guards[i].bytes,16)){fprintf(stderr,"{\"event\":\"fm5_luminance_readback_init\",\"status\":\"guard_failed\",\"rva\":\"0x%x\"}\n",guards[i].rva);fflush(stderr);return TRUE;}
 barriers=(Barriers)(host+0xee080);copy=(Copy)(host+0xee420);frequency=f.QuadPart;enabled=TRUE;
 fprintf(stderr,"{\"event\":\"fm5_luminance_readback_init\",\"status\":\"enabled\",\"deadline_ms\":2000,\"max_inflight\":1}\n");fflush(stderr);return TRUE;
}
static HRESULT plan(const D3D12_RESOURCE_DESC &desc,UINT state,D3D12_RESOURCE_BARRIER *out){
 if(desc.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || desc.Width!=1 || desc.Height!=1 ||
    desc.DepthOrArraySize!=1 || desc.MipLevels!=1 || desc.Format!=DXGI_FORMAT_R32_FLOAT || desc.SampleDesc.Count!=1 || desc.SampleDesc.Quality ||
    (desc.Flags&D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL) || state!=0xc0)return E_NOTIMPL;
 ZeroMemory(out,2*sizeof(*out));out[0].Type=out[1].Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
 out[0].Transition.Subresource=out[1].Transition.Subresource=0;
 out[0].Transition.StateBefore=(D3D12_RESOURCE_STATES)state;
 out[0].Transition.StateAfter=(D3D12_RESOURCE_STATES)(state|D3D12_RESOURCE_STATE_COPY_SOURCE);
 out[1].Transition.StateBefore=out[0].Transition.StateAfter;out[1].Transition.StateAfter=out[0].Transition.StateBefore;return S_OK;
}
static BOOL poll_job(){
  AcquireSRWLockExclusive(&lock);
  if(job.phase==Idle || job.phase==Quarantined){ReleaseSRWLockExclusive(&lock);return TRUE;}
  if(job.phase==Submitted){
   UINT64 done=job.fence->GetCompletedValue();
   if(done>=1){
    HRESULT hr=done==UINT64_MAX?DXGI_ERROR_DEVICE_REMOVED:S_OK;UINT bits=0;
    if(SUCCEEDED(hr)){D3D12_RANGE range={(SIZE_T)job.footprint.Offset,(SIZE_T)job.footprint.Offset+4};void *mapped=NULL;
     hr=job.readback->Map(0,&range,&mapped);
     if(SUCCEEDED(hr)&&mapped){memcpy(&bits,(BYTE*)mapped+job.footprint.Offset,4);D3D12_RANGE none={0,0};job.readback->Unmap(0,&none);}
     else if(SUCCEEDED(hr))hr=E_UNEXPECTED;
    }
#ifdef FM5_LUMINANCE_READBACK_TEST
    result_valid=SUCCEEDED(hr);last_result=bits;
#endif
    log_event(&job,SUCCEEDED(hr)?"complete":"failed",hr,SUCCEEDED(hr)?&bits:NULL);clean(&job);InterlockedExchange(&active,0);ReleaseSRWLockExclusive(&lock);return TRUE;
   }
  }
  if((DWORD)(GetTickCount()-job.deadline)>=2000){
   log_event(&job,"timeout_quarantined",HRESULT_FROM_WIN32(WAIT_TIMEOUT));job.phase=Quarantined;InterlockedExchange(&active,0);
   /* In-flight resources are deliberately retained; a timeout never proves GPU retirement. Max one job. */
   ReleaseSRWLockExclusive(&lock);return TRUE;
  }
  ReleaseSRWLockExclusive(&lock);return FALSE;
}
#ifndef FM5_LUMINANCE_READBACK_TEST
static DWORD WINAPI worker(void*){while(!poll_job())Sleep(2);return 0;}
#endif
#ifdef FM5_LUMINANCE_READBACK_TEST
static BOOL test_thread_fail;
#endif
static BOOL start_worker(){
#ifdef FM5_LUMINANCE_READBACK_TEST
 if(test_thread_fail){SetLastError(ERROR_NOT_ENOUGH_MEMORY);return FALSE;}return TRUE;
#else
 HANDLE thread=CreateThread(NULL,0,worker,NULL,0,NULL);if(!thread)return FALSE;CloseHandle(thread);return TRUE;
#endif
}
}

extern "C" __declspec(dllexport) void WINAPI FM5ReadbackAfterPreparation(void *context,void *abstract_list){
 DWORD error=GetLastError();InitOnceExecuteOnce(&once,initialize,NULL,NULL);if(!enabled){SetLastError(error);return;}
 AcquireSRWLockExclusive(&lock);
 if(job.phase!=Idle || captures>=1200){ReleaseSRWLockExclusive(&lock);SetLastError(error);return;}
 LARGE_INTEGER now;QueryPerformanceCounter(&now);
 if(last_request && now.QuadPart-last_request<frequency){ReleaseSRWLockExclusive(&lock);SetLastError(error);return;}
 Job local={};local.requested=now.QuadPart;D3D12_RESOURCE_BARRIER transitions[2];HRESULT hr=E_NOTIMPL;volatile BOOL transferred=FALSE,ready=FALSE,matched=FALSE;const char *reason="refused_context";
 __try {
  void *device=pointer(context,0xc40);BYTE modes[2]={0xff,0xff};
  void *ps=pointer(context,0x40f0),*vs=pointer(context,0x40d0),*psprog=pointer(ps,0x58),*vsprog=pointer(vs,0x58);
  static const BYTE pskey[20]={0x5c,0x71,0xc8,0x21,0xe8,0x35,0xb3,0x48,0x3f,0xca,0x11,0x0f,0xba,0x27,0x26,0x5b,0x54,0x14,0x01,0x00};
  static const BYTE vskey[20]={0x06,0x8a,0x59,0x74,0x60,0x71,0x6c,0xd5,0xe1,0xe9,0x7a,0x48,0xfd,0xf3,0x53,0xc4,0x14,0xf6,0x00,0x00};
  BYTE key[20];if(!read(psprog,0x168,key,20)||memcmp(key,pskey,20)||!read(vsprog,0x168,key,20)||memcmp(key,vskey,20))__leave;
  matched=TRUE;
  if(!device || (BYTE*)context!=(BYTE*)device+0x6f40 || abstract_list!=(BYTE*)context+0x9d58 ||
     !read(context,0xc48,modes,2) || modes[0] || modes[1] || pointer(context,0x9d98))__leave;
  void **context_vt=(void**)pointer(context,0);
  if(!context_vt || context_vt[3]!=(void*)(host+0xdbe90))__leave;
  reason="refused_view";
  local.native_view=pointer(context,0x4100+4*8);void *view=local.native_view;
  if(!view || pointer(view,0x30)!=device || word(view,0x38)!=11 ||
     pointer(pointer(view,0),8*8)!=(void*)(host+0x141910) || word(view,0x108)!=41 || word(view,0x10c)!=4 || word(view,0x110)!=0 || word(view,0x114)!=1)__leave;
  reason="refused_resource";local.native_resource=pointer(view,0x48);void *resource=local.native_resource;
  if(!resource || pointer(resource,0x30)!=device || word(resource,0x38)!=12 ||
     pointer(pointer(resource,0),12*8)!=(void*)(host+0xeab90))__leave;
  reason="refused_view_backing_identity";void *allocation=pointer(view,0x50);
  if(!allocation || allocation!=pointer(resource,0x40))__leave;
  reason="refused_state_layout";BYTE stateless=1;if(!read(allocation,0x30,&stateless,1)||stateless)__leave;
  BYTE *states=(BYTE*)pointer(allocation,0x18);
  if(!states || pointer(allocation,0x20)!=states+4)__leave;
  local.original_state=word(states,0);
  local.physical=(ID3D12Resource*)pointer(allocation,0x10);if(!local.physical)__leave;
  reason="refused_physical_desc_or_state";D3D12_RESOURCE_DESC desc=local.physical->GetDesc();hr=plan(desc,local.original_state,transitions);if(FAILED(hr))__leave;
  reason="refused_direct_queue";
  local.queue=(ID3D12CommandQueue*)pointer(device,0xcd0);if(!local.queue || local.queue->GetDesc().Type!=D3D12_COMMAND_LIST_TYPE_DIRECT)__leave;
  reason="refused_gpu_device";hr=local.physical->GetDevice(__uuidof(ID3D12Device),(void**)&local.device);if(FAILED(hr)||!local.device)__leave;
  D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint={};UINT rows=0;UINT64 row_bytes=0,total=0;
  local.device->GetCopyableFootprints(&desc,0,1,0,&footprint,&rows,&row_bytes,&total);
  reason="refused_footprint";if(rows!=1 || row_bytes!=4 || footprint.Offset || footprint.Footprint.RowPitch!=256 || total>4096 || total<4){hr=E_UNEXPECTED;__leave;}
  local.footprint=footprint;
  D3D12_HEAP_PROPERTIES heap={};heap.Type=D3D12_HEAP_TYPE_READBACK;heap.CreationNodeMask=heap.VisibleNodeMask=1;
  D3D12_RESOURCE_DESC buffer={};buffer.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;buffer.Width=256;buffer.Height=1;
  buffer.DepthOrArraySize=buffer.MipLevels=1;buffer.SampleDesc.Count=1;buffer.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  reason="refused_readback_factory";hr=local.device->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&buffer,D3D12_RESOURCE_STATE_COPY_DEST,NULL,__uuidof(ID3D12Resource),(void**)&local.readback);
  if(FAILED(hr)||!local.readback)__leave;
  reason="refused_fence_factory";hr=local.device->CreateFence(0,D3D12_FENCE_FLAG_NONE,__uuidof(ID3D12Fence),(void**)&local.fence);if(FAILED(hr)||!local.fence)__leave;
  native_addref(local.native_view);native_addref(local.native_resource);local.physical->AddRef();local.queue->AddRef();
  local.serial=++serial;local.requested=now.QuadPart;local.deadline=GetTickCount();local.phase=Building;job=local;transferred=TRUE;InterlockedExchange(&active,1);
  reason="refused_worker_start";if(!start_worker()){hr=HRESULT_FROM_WIN32(GetLastError());clean(&job);InterlockedExchange(&active,0);ZeroMemory(&local,sizeof(local));transferred=FALSE;__leave;}
  last_request=now.QuadPart;++captures;ready=TRUE;
 } __except(EXCEPTION_EXECUTE_HANDLER){hr=E_UNEXPECTED;}
 if(!ready){
 if(transferred){job.phase=Quarantined;InterlockedExchange(&active,0);log_event(&job,"prepare_exception_quarantined",hr);}
 else if(matched && refusals<120 && (!last_refusal || now.QuadPart-last_refusal>=frequency)){last_refusal=now.QuadPart;++refusals;log_event(&local,reason,hr);}
 /* Only device/readback/fence have acquired refs before the ownership transfer. */
 if(!transferred){if(local.device)local.device->Release();if(local.readback)local.readback->Release();if(local.fence)local.fence->Release();}
 ReleaseSRWLockExclusive(&lock);SetLastError(error);return;
 }
 ReleaseSRWLockExclusive(&lock);
 /* This SEH region always starts and ends WITHOUT the job lock. It cannot
    unwind through a lock release or a change of resource ownership. */
 __try {
  transitions[0].Transition.pResource=transitions[1].Transition.pResource=local.physical;
  D3D12_TEXTURE_COPY_LOCATION dst={};dst.pResource=local.readback;dst.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst.PlacedFootprint=local.footprint;
  D3D12_TEXTURE_COPY_LOCATION src={};src.pResource=local.physical;src.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;src.SubresourceIndex=0;
  barriers(abstract_list,1,&transitions[0]);copy(abstract_list,&dst,0,0,0,&src,NULL);barriers(abstract_list,1,&transitions[1]);
 } __except(EXCEPTION_EXECUTE_HANDLER){
  AcquireSRWLockExclusive(&lock);job.phase=Quarantined;InterlockedExchange(&active,0);log_event(&job,"record_exception_quarantined",E_UNEXPECTED);ReleaseSRWLockExclusive(&lock);SetLastError(error);return;
 }
 AcquireSRWLockExclusive(&lock);log_event(&job,"enqueued",S_OK);ReleaseSRWLockExclusive(&lock);SetLastError(error);
}

extern "C" __declspec(dllexport) void WINAPI FM5ReadbackCopyTextureRegion(void *raw_list,const void *raw_dst,UINT x,UINT y,UINT z,const void *raw_src,const void *raw_box){
 ID3D12GraphicsCommandList *list=(ID3D12GraphicsCommandList*)raw_list;
 list->CopyTextureRegion((const D3D12_TEXTURE_COPY_LOCATION*)raw_dst,x,y,z,(const D3D12_TEXTURE_COPY_LOCATION*)raw_src,(const D3D12_BOX*)raw_box);
 if(!InterlockedCompareExchange(&active,0,0))return;
 DWORD error=GetLastError();AcquireSRWLockExclusive(&lock);
 const D3D12_TEXTURE_COPY_LOCATION *dst=(const D3D12_TEXTURE_COPY_LOCATION*)raw_dst;
 if(dst && job.phase!=Idle && job.phase!=Quarantined && dst->pResource==job.readback){
  if(job.list && job.list!=raw_list){job.phase=Quarantined;InterlockedExchange(&active,0);log_event(&job,"multiple_lists_quarantined",E_UNEXPECTED);}
  else {job.list=raw_list;if(job.phase==Building)job.phase=Recorded;}
 }
 ReleaseSRWLockExclusive(&lock);SetLastError(error);
}
extern "C" void WINAPI FM5ReadbackSubmitted(void *raw_queue,UINT count,void *const *lists){
 if(!InterlockedCompareExchange(&active,0,0))return;
 DWORD error=GetLastError();AcquireSRWLockExclusive(&lock);
 if(job.phase==Recorded && job.list && raw_queue==job.queue && lists){
  UINT matches=0;if(count>4096){job.phase=Quarantined;InterlockedExchange(&active,0);log_event(&job,"batch_count_quarantined",E_INVALIDARG);ReleaseSRWLockExclusive(&lock);SetLastError(error);return;}
  for(UINT i=0;i<count;++i)if(lists[i]==job.list)++matches;
  if(matches==1){HRESULT hr=job.queue->Signal(job.fence,1);if(SUCCEEDED(hr))job.phase=Submitted;
   else {job.phase=Quarantined;InterlockedExchange(&active,0);log_event(&job,"signal_failed_quarantined",hr);}}
  else if(matches>1){job.phase=Quarantined;InterlockedExchange(&active,0);log_event(&job,"duplicate_list_quarantined",E_UNEXPECTED);}
 }
 ReleaseSRWLockExclusive(&lock);SetLastError(error);
}

#ifdef FM5_LUMINANCE_READBACK_TEST
extern "C" HRESULT FM5ReadbackTestPlan(const D3D12_RESOURCE_DESC *desc,UINT state,D3D12_RESOURCE_BARRIER *out){return plan(*desc,state,out);}
extern "C" void FM5ReadbackTestConfigure(const void *base,Barriers b,Copy c){
 AcquireSRWLockExclusive(&lock);clean(&job);InterlockedExchange(&active,0);host=(const BYTE*)base;barriers=b;copy=c;enabled=TRUE;captures=0;last_request=0;last_refusal=0;refusals=0;
 LARGE_INTEGER f;QueryPerformanceFrequency(&f);frequency=f.QuadPart;test_thread_fail=FALSE;result_valid=FALSE;ReleaseSRWLockExclusive(&lock);
}
extern "C" UINT FM5ReadbackTestPhase(){return (UINT)job.phase;}
extern "C" void FM5ReadbackTestStartFailure(BOOL fail){test_thread_fail=fail;}
extern "C" BOOL FM5ReadbackTestPoll(){return poll_job();}
extern "C" void FM5ReadbackTestExpire(){job.deadline=GetTickCount()-2001;}
extern "C" UINT FM5ReadbackTestGuardBytes(const BYTE *base,SIZE_T size){
 for(UINT i=0;i<_countof(guards);++i)if(size<guards[i].rva+16 || memcmp(base+guards[i].rva,guards[i].bytes,16))return i+1;
 return 0;
}
extern "C" BOOL FM5ReadbackTestArm(void *physical,void *readback,void *queue,void *fence){
 AcquireSRWLockExclusive(&lock);if(job.phase!=Idle){ReleaseSRWLockExclusive(&lock);return FALSE;}
 job.physical=(ID3D12Resource*)physical;job.readback=(ID3D12Resource*)readback;job.queue=(ID3D12CommandQueue*)queue;job.fence=(ID3D12Fence*)fence;
 job.physical->AddRef();job.readback->AddRef();job.queue->AddRef();job.fence->AddRef();job.phase=Building;job.serial=++serial;InterlockedExchange(&active,1);
 LARGE_INTEGER n,f;QueryPerformanceCounter(&n);QueryPerformanceFrequency(&f);job.requested=n.QuadPart;frequency=f.QuadPart;job.deadline=GetTickCount();job.original_state=0xc0;
 result_valid=FALSE;ReleaseSRWLockExclusive(&lock);return TRUE;
}
extern "C" BOOL FM5ReadbackTestResult(UINT *bits){if(bits)*bits=last_result;return result_valid;}
#endif
