#define WIN32_LEAN_AND_MEAN
#include "fm5_color_observer.h"
#ifndef FM5_COLOR_OBSERVER_TEST
#include "../allocation-port/fm5_luminance_readback.h"
#endif
#pragma warning(push)
#pragma warning(disable:4201)
#include <d3d12.h>
#pragma warning(pop)
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
extern "C" void *FM5UnwrapPerformanceContext(void *);
extern "C" void FM5DiagnosticWriteJsonLine(const char *,DWORD);

namespace {
struct Guard { SIZE_T rva; BYTE bytes[16]; };
static const Guard guards[]={
 {0x63550,{0x48,0x85,0xd2,0x0f,0x84,0x2b,0x01,0x00,0x00,0x48,0x8b,0x41,0x40,0x48,0x3b,0x05}},
 {0x1b3b10,{0x40,0x53,0x55,0x56,0x57,0x41,0x54,0x41,0x56,0x41,0x57,0x48,0x81,0xec,0xb0,0x01}},
 {0x1b29a0,{0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x41,0x54,0x41,0x55,0x41,0x56,0x48,0x83,0xec}},
 {0x1a79e0,{0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0x6c}},
 {0x1a7d90,{0xc2,0x00,0x00,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc,0xcc}},
 {0xe9d50,{0x48,0x89,0x5c,0x24,0x18,0x55,0x56,0x57,0x48,0x83,0xec,0x70,0x48,0x8b,0x05,0xdd}},
 {0x12a6d0,{0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57}},
 {0xe6bf0,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x48}},
 {0xe8fa0,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x18,0x56,0x57,0x41,0x56,0x48,0x83}},
 {0xe95c0,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57}},
 {0x1b2ca0,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48}},
 {0x1b2390,{0x8b,0x55,0x6f,0x03,0xd3,0x8b,0x45,0x67,0x48,0xc1,0xe0,0x07,0x48,0x03,0xd0,0x48}},
 {0x1b2550,{0x89,0x54,0x24,0x10,0x55,0x41,0x54,0x41,0x57,0x48,0x83,0xec,0x60,0x8b,0xea,0x4c}},
 {0x141910,{0x48,0x85,0xd2,0x74,0x2b,0x57,0x48,0x83,0xec,0x20,0x48,0x89,0x5c,0x24,0x30,0x48}},
 {0xeab90,{0x48,0x85,0xd2,0x74,0x35,0x0f,0x10,0x41,0x70,0x0f,0x11,0x02,0x0f,0x10,0x89,0x80}},
 {0xea000,{0x48,0x8b,0xc4,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48}},
 {0x1b5290,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57}},
 {0xc2f00,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x4c,0x24,0x08,0x57}},
 {0x66862,{0x49,0x8d,0x8d,0x68,0x01,0x00,0x00,0x0f,0x10,0x44,0x24,0x58,0x0f,0x11,0x01,0xf2}}
};
struct Line { char *bytes; SIZE_T used,capacity; BOOL truncated; };
#ifndef FM5_COLOR_OBSERVER_TEST
static INIT_ONCE once=INIT_ONCE_STATIC_INIT;
#endif
#ifndef FM5_COLOR_OBSERVER_TEST
struct FM5ColorLock {void *mutex; BYTE held; BYTE reserved[7];};
#endif
typedef FM5ColorLock *(*AcquireLock)(FM5ColorLock *,void *);
static AcquireLock acquire_lock;
static SRWLOCK rate_lock=SRWLOCK_INIT;
static const BYTE *host;
static BOOL enabled;
static void *expected_ui;
static LONGLONG frequency,last_tick;
static UINT snapshots;
static void *(*unwrap)(void *)=FM5UnwrapPerformanceContext;
static void (*writer)(const char *,DWORD)=FM5DiagnosticWriteJsonLine;
static LONGLONG clock_now() { LARGE_INTEGER value;QueryPerformanceCounter(&value);return value.QuadPart; }
static LONGLONG (*clock_read)(void)=clock_now;
typedef void *(WINAPI *NativePreparation)(void *,BYTE,void *);
static NativePreparation original_preparation;
#ifndef FM5_COLOR_OBSERVER_TEST
static INIT_ONCE preparation_once=INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK resolve_preparation(PINIT_ONCE,void *,void **) {
 const BYTE *base=(const BYTE*)GetModuleHandleW(NULL);
 if(base)original_preparation=(NativePreparation)(base+0x12a6d0);
 return TRUE;
}
#endif
static UINT word(const void *,SIZE_T);
struct Family {UINT size;BYTE checksum[16];UINT family,rva;UINT64 fnv;};
static const Family families[]={
#include "../audio-tls/color-pipeline/fm5_postprocess_families.inc"
};
struct TaggedProgram {BYTE key[20];UINT family,rva;LONGLONG last;UINT samples;};
struct Upload {void *resource,*owner,*context,*mapped,*mapped_address;UINT id,thread,map_epoch,unmap_epoch;BOOL valid;LONGLONG tick;BYTE gpu_address[8],bytes[1664];};
static SRWLOCK diagnostic_lock=SRWLOCK_INIT;
static TaggedProgram tagged[32];static UINT tagged_count;
struct DrawPair {void *context;BOOL active;LONG64 id;TaggedProgram program;};
static __declspec(thread) DrawPair draw_pairs[8];
static __declspec(thread) UINT draw_depth;
static volatile LONG64 pair_sequence;
static Upload uploads[64];static UINT next_upload;
static UINT classify(const void *data,SIZE_T size,UINT *rva) {
 if(!data || size<32 || memcmp(data,"DXBC",4) || word(data,24)!=size)return 0;
 for(UINT i=0;i<_countof(families);++i) {
  const Family *candidate=&families[i];
  if(size!=candidate->size || memcmp((const BYTE*)data+4,candidate->checksum,16))continue;
  UINT64 fnv=14695981039346656037ULL;
  for(SIZE_T n=0;n<size;++n){fnv^=((const BYTE*)data)[n];fnv*=1099511628211ULL;}
  if(fnv==candidate->fnv){if(rva)*rva=candidate->rva;return candidate->family;}
 }
 return 0;
}

static HRESULT validate(const BYTE *base,SIZE_T size) {
 if(!base)return E_POINTER;
 if(size<0x29fbb8)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
 for(UINT i=0;i<_countof(guards);++i)
  if(size<guards[i].rva+16 || memcmp(base+guards[i].rva,guards[i].bytes,16))
   return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
 return S_OK;
}
#ifndef FM5_COLOR_OBSERVER_TEST
static BOOL CALLBACK initialize(PINIT_ONCE,void *,void **) {
 WCHAR option[2];LARGE_INTEGER qpf;
 if(GetEnvironmentVariableW(L"FM5_COLOR_DIAGNOSTICS",option,2)!=1 || option[0]!=L'1')return TRUE;
 host=(const BYTE*)GetModuleHandleW(NULL);
 if(!host)return TRUE;
 const IMAGE_NT_HEADERS64 *pe=(const IMAGE_NT_HEADERS64*)(host+((const IMAGE_DOS_HEADER*)host)->e_lfanew);
 if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 ||
    FAILED(validate(host,pe->OptionalHeader.SizeOfImage)) || !QueryPerformanceFrequency(&qpf))return TRUE;
 frequency=qpf.QuadPart;enabled=frequency>0;
 acquire_lock=(AcquireLock)(host+0xc2f00);
 return TRUE;
}
#endif
static void append(Line *line,const char *format,...) {
 if(line->truncated)return;
 va_list args;va_start(args,format);
 int count=_vsnprintf_s(line->bytes+line->used,line->capacity-line->used,_TRUNCATE,format,args);
 va_end(args);
 if(count<0)line->truncated=TRUE;else line->used+=(SIZE_T)count;
}
static UINT word(const void *base,SIZE_T offset) {UINT value;memcpy(&value,(const BYTE*)base+offset,4);return value;}
static void *pointer(const void *base,SIZE_T offset) {void *value;memcpy(&value,(const BYTE*)base+offset,8);return value;}
static void hex(Line *line,const void *data,SIZE_T count) {
 const BYTE *bytes=(const BYTE*)data;
 append(line,"\"");for(SIZE_T i=0;i<count;++i)append(line,"%02x",bytes[i]);append(line,"\"");
}
static void resource(Line *line,void *value,void *device) {
 if(!value){append(line,"null");return;}
 BYTE *object=(BYTE*)value;UINT kind=word(object,0x38);
 append(line,"{\"pointer\":\"%p\",\"kind\":%u,\"same_owner\":%s",value,kind,
    pointer(object,0x30)==device?"true":"false");
 if(pointer(object,0x30)!=device){append(line,"}");return;}
 UINT words=kind==5?6:kind==12?11:kind==13?9:kind==16?8:0;
 if(words){
  append(line,",\"requested_description\":[");
  for(UINT i=0;i<words;++i)append(line,"%s%u",i?",":"",word(object,0x70+i*4));
  append(line,"]");
 }
 /* eab90 and ea000 pin the Texture2D wrapper/backing layout. GetDesc is a
    real immutable D3D12 metadata query; it does not submit/wait GPU work. */
 if(kind==12 && (*(void***)object)[12]==(void*)(host+0xeab90)) {
  void *backing=pointer(object,0x40);
  ID3D12Resource *physical=backing?(ID3D12Resource*)pointer(backing,0x10):NULL;
  if(physical) {
   D3D12_RESOURCE_DESC description=physical->GetDesc();
   append(line,",\"physical_resource\":\"%p\",\"actual_description\":{\"dimension\":%u,"
    "\"width\":%llu,\"height\":%u,\"depth_or_array\":%u,\"mips\":%u,\"format\":%u,"
    "\"samples\":%u,\"quality\":%u,\"flags\":%u}",physical,(UINT)description.Dimension,
    (unsigned long long)description.Width,description.Height,(UINT)description.DepthOrArraySize,
    (UINT)description.MipLevels,(UINT)description.Format,description.SampleDesc.Count,
    description.SampleDesc.Quality,(UINT)description.Flags);
  }
 }
 append(line,"}");
}
static void shader(Line *line,void *value,void *device,UINT stage) {
 if(!value){append(line,"null");return;}
 append(line,"{\"pointer\":\"%p\",\"kind\":%u,\"stage\":%u",value,word(value,0x38),word(value,0x50));
 if(word(value,0x38)==8 && pointer(value,0x30)==device && word(value,0x50)==stage) {
  void *program=pointer(value,0x58);
  append(line,",\"program\":\"%p\"",program);
  if(program){
   /* CreatePixelShader667f8..66877 stores its two native byte-vector FNV
      accumulators and length at168. Keep this distinct from layout hashes. */
   append(line,",\"native_bytecode_key_at168\":");hex(line,(BYTE*)program+0x168,24);
   append(line,",\"binding_signatures_at180\":");hex(line,(BYTE*)program+0x180,32);
  }
 }
 append(line,"}");
}
static void view_allocations(Line *line,void *view,void *device) {
 UINT kind=word(view,0x38);BOOL same_owner=pointer(view,0x30)==device;
 BOOL getter=(*(void***)view)[8]==(void*)(host+0x141910);
 append(line,",\"view_kind\":%u,\"view_owner_matches\":%s,\"native_get_resource_verified\":%s",
  kind,same_owner?"true":"false",getter?"true":"false");
 if(kind!=11 || !same_owner || !getter)return;
 void *logical=pointer(view,0x48);BOOL logical_owner=logical && pointer(logical,0x30)==device;
 UINT resource_kind=logical?word(logical,0x38):0;
 BOOL layout=logical_owner && (resource_kind==5 || resource_kind==12 || resource_kind==13 || resource_kind==16);
 append(line,",\"logical_owner_matches\":%s,\"logical_resource_kind\":%u,\"allocation_layout_verified\":%s",
  logical_owner?"true":"false",resource_kind,layout?"true":"false");
 if(!layout)return;
 void *cached=pointer(view,0x50),*backing=pointer(logical,0x40);
 void *cached_physical=cached?pointer(cached,0x10):NULL,*logical_physical=backing?pointer(backing,0x10):NULL;
 append(line,",\"view_cached_backing\":\"%p\",\"view_cached_physical_resource\":\"%p\","
  "\"logical_backing\":\"%p\",\"logical_physical_resource\":\"%p\",\"cached_and_logical_physical_match\":",
  cached,cached_physical,backing,logical_physical);
 if(cached_physical && logical_physical)append(line,"%s",cached_physical==logical_physical?"true":"false");else append(line,"null");
 append(line,",\"resource_dynamic_flags_hex\":");hex(line,(BYTE*)logical+0x88,2);
}
static void stage(Line *line,BYTE *context,void *device,UINT index) {
 append(line,"{\"stage\":%u,\"shader\":",index);
 shader(line,pointer(context,0x40d0+index*8),device,index);
 append(line,",\"srv_pointers\":[");
 for(UINT i=0;i<128;++i)append(line,"%s\"%p\"",i?",":"",pointer(context,0x4100+(index*128+i)*8));
 append(line,"],\"srv_metadata\":[");
 UINT captured=0;
 for(UINT i=0;i<128 && captured<32;++i) {
  void *view=pointer(context,0x4100+(index*128+i)*8);
  if(!view)continue;
  append(line,"%s{\"slot\":%u,\"view\":\"%p\"",captured?",":"",i,view);++captured;
  view_allocations(line,view,device);
  if(pointer(view,0x30)==device && (*(void***)view)[8]==(void*)(host+0x141910)) {
   append(line,",\"view_descriptor\":[");
   for(UINT n=0;n<6;++n)append(line,"%s%u",n?",":"",word(view,0x108+n*4));
   append(line,"],\"resource\":");resource(line,pointer(view,0x48),device);
  } else append(line,",\"unverified_view\":true");
  append(line,"}");
 }
 append(line,"],\"srv_metadata_limit\":32,\"logical_cbv_cache_active\":%s,\"cbv\":[",word(context,0xd2c0)==0?"true":"false");
 for(UINT i=0;i<14;++i) {
  SIZE_T ordinal=index*14+i;
  append(line,"%s{\"slot\":%u,\"backend_entry_hex\":",i?",":"",i);
  hex(line,context+0x5900+ordinal*40,40);
  append(line,",\"logical_entry_hex\":");hex(line,context+0x8a48+ordinal*16,16);
  if(!word(context,0xd2c0)) {
   append(line,",\"logical_buffer\":");resource(line,pointer(context,0x8a48+ordinal*16),device);
  }
  append(line,"}");
 }
 append(line,"]}");
}
static void capture(Line *line,void *ui,LONGLONG tick) {
 void *proxy=pointer(ui,0x18);BYTE *context=proxy?(BYTE*)unwrap(proxy):NULL;
 append(line,"{\"event\":\"fm5_color_before_ui\",\"qpc\":%lld,\"qpc_frequency\":%lld,"
    "\"thread\":%lu,\"ui\":\"%p\",\"proxy\":\"%p\",\"native\":\"%p\"",tick,frequency,
    GetCurrentThreadId(),ui,proxy,context);
 if(!context || (*(void***)context)[3]!=(void*)(host+0xdbe90)) {
  append(line,",\"unverified_context\":true}\n");return;
 }
 if(context[0xc48]!=1 || context[0xc49]!=0) {
  append(line,",\"unsupported_cache_mode\":[%u,%u]}\n",context[0xc48],context[0xc49]);return;
 }
 FM5ColorLock guard={};
 acquire_lock(&guard,context);
 __try {
 void *device=pointer(context,0xc40);
 append(line,",\"device\":\"%p\",\"snapshot_atomic\":false,\"context_lock_held\":true,\"sample_mask\":%u,"
    "\"blend_state\":\"%p\",\"blend_factor_hex\":",device,word(context,0x6fa0),pointer(context,0x6f88));
 hex(line,context+0x6f90,16);
 append(line,",\"stages\":[");stage(line,context,device,0);append(line,",");stage(line,context,device,4);
 append(line,"]}\n");
 } __finally {
  if(guard.held) {
   BYTE *mutex=(BYTE*)guard.mutex;
   if(--*(UINT*)(mutex+12)==0) {
    *(UINT*)(mutex+8)=0;ReleaseSRWLockExclusive((PSRWLOCK)guard.mutex);
   }
   guard.held=0;
  }
 }
}

static void release_native_lock(FM5ColorLock *guard) {
 if(guard->held){BYTE *mutex=(BYTE*)guard->mutex;
  if(--*(UINT*)(mutex+12)==0){*(UINT*)(mutex+8)=0;ReleaseSRWLockExclusive((PSRWLOCK)mutex);}
  guard->held=0;
 }
}
static BOOL initialize_enabled() {
#ifndef FM5_COLOR_OBSERVER_TEST
 InitOnceExecuteOnce(&once,initialize,NULL,NULL);
#endif
 return enabled;
}
static BOOL candidate_buffer(void *value,void *device) {
 return value && word(value,0x38)==5 && pointer(value,0x30)==device &&
  word(value,0x70)==1664 && word(value,0x74)==2 &&
  word(value,0x78)==4 && word(value,0x7c)==0x10000;
}
static BOOL safe_read(const void *base,SIZE_T offset,void *out,SIZE_T count) {
 uintptr_t address=(uintptr_t)base;
 if(!address || offset>UINTPTR_MAX-address || count>UINTPTR_MAX-address-offset)return FALSE;
 SIZE_T copied=0;return ReadProcessMemory(GetCurrentProcess(),(const void*)(address+offset),out,count,&copied)&&copied==count;
}
struct BoundIdentity {void *owner,*backing,*allocation;UINT kind,id,description[6];UINT64 gpu;};
static BOOL bound_identity(void *buffer,BoundIdentity *identity) {
 BYTE bytes[0x150];if(!safe_read(buffer,0,bytes,sizeof(bytes)))return FALSE;
 ZeroMemory(identity,sizeof(*identity));identity->owner=pointer(bytes,0x30);identity->kind=word(bytes,0x38);
 identity->id=word(bytes,0x3c);identity->backing=pointer(bytes,0x40);identity->allocation=pointer(bytes,0x140);
 memcpy(identity->description,bytes+0x70,sizeof(identity->description));memcpy(&identity->gpu,bytes+0x148,8);return TRUE;
}
static void actual_bound_payload(Line *line,BYTE *context,void *device) {
 BYTE ps[40],vs[40],record[0x38],again_record[0x38],again_ps[40],again_vs[40],payload[1664];
 BoundIdentity before={},after={};const char *reason="unverified_binding";
 void *buffer=NULL,*allocation=NULL,*cpu=NULL;UINT64 gpu=0;
 const SIZE_T ps_at=0x5900+(4*14+1)*40,vs_at=0x5900+1*40;
 if(context[0xc48]!=0 || context[0xc49]!=0 || (uintptr_t)context!=(uintptr_t)device+0x6f40){reason="not_prepared_immediate_context";goto unavailable;}
 if(!safe_read(context,ps_at,ps,40)||!safe_read(context,vs_at,vs,40))goto unavailable;
 buffer=pointer(ps,0);if(!buffer || pointer(vs,0)!=buffer){reason="VS_PS_buffers_differ";goto unavailable;}
 if(word(ps,8)||word(vs,8) || (word(ps,12)&&word(ps,12)<104) || (word(vs,12)&&word(vs,12)<104)){
  reason="bound_range_does_not_cover_1664_bytes_at_zero";goto unavailable;
 }
 if(!bound_identity(buffer,&before) || before.owner!=device || before.kind!=5 ||
    before.description[0]<sizeof(payload) || before.description[1]!=2 || before.description[2]!=4 || before.description[3]!=0x10000){
  reason="unverified_dynamic_constant_buffer";goto unavailable;
 }
 allocation=before.allocation;
 if(!allocation || !safe_read(allocation,0,record,sizeof(record))){reason="no_current_GPU_allocation_record";goto unavailable;}
 cpu=pointer(record,0x20);memcpy(&gpu,record+0x28,8);
 if(!cpu || !gpu || pointer(record,0x30)!=before.backing || gpu!=before.gpu ||
    memcmp(record+0x28,ps+24,8)||memcmp(record+0x28,ps+32,8)||
    memcmp(record+0x28,vs+24,8)||memcmp(record+0x28,vs+32,8)){
  reason="allocation_GPU_VA_does_not_match_both_prepared_CBVs";goto unavailable;
 }
 if(!safe_read(cpu,0,payload,sizeof(payload))){reason="allocation_CPU_payload_unreadable";goto unavailable;}
 if(!bound_identity(buffer,&after) || memcmp(&before,&after,sizeof(before)) ||
    !safe_read(allocation,0,again_record,sizeof(again_record)) ||
    memcmp(record+0x20,again_record+0x20,24) ||
    !safe_read(context,ps_at,again_ps,40)||memcmp(ps,again_ps,40)||
    !safe_read(context,vs_at,again_vs,40)||memcmp(vs,again_vs,40)){
  reason="binding_or_allocation_changed_during_CPU_copy";goto unavailable;
 }
 append(line,",\"actual_cpu_bound_bytes\":{\"buffer\":\"%p\",\"resource_id\":%u,\"allocation\":\"%p\","
  "\"cpu_address\":\"%p\",\"gpu_va\":%llu,\"repeat_identity_stable\":true,\"gpu_readback\":false,"
  "\"provenance\":\"resource140_current_allocation_matching_VS_PS_prepared_CBVs\",\"bytes\":",
  buffer,before.id,allocation,cpu,(unsigned long long)gpu);
 hex(line,payload,sizeof(payload));append(line,"}");return;
unavailable:
 append(line,",\"actual_cpu_bound_bytes\":null,\"actual_cpu_bound_reason\":\"%s\"",reason);
}
static void renamed_allocation(Line *line,BYTE *context,void *buffer) {
 /* Read-only equivalent of e9d86..e9e48. Never call e9d50: a miss inserts. */
 void *owner=NULL,*buckets=NULL,*sentinel=NULL,*node=NULL,*endpoint=NULL;UINT64 mask=0;
 if(!safe_read(context,0xd330,&owner,8) ||
    !safe_read(owner,0x250,&buckets,8)||!safe_read(owner,0x268,&mask,8)||!safe_read(owner,0x240,&sentinel,8)){
  append(line,",\"renamed_allocation\":null");return;
 }
 UINT key=word(buffer,0x3c);UINT64 hash=14695981039346656037ULL;
 for(UINT n=0;n<4;++n){hash^=(key>>(n*8))&255;hash*=1099511628211ULL;}
 UINT64 index=hash&mask;
 if(index>(SIZE_MAX-8)/16 || !safe_read(buckets,(SIZE_T)index*16+8,&node,8)||
    !safe_read(buckets,(SIZE_T)index*16,&endpoint,8)){append(line,",\"renamed_allocation\":null");return;}
 for(UINT n=0;n<64 && node && node!=sentinel;++n){UINT found_key=0;
  if(!safe_read(node,0x10,&found_key,4))break;
  if(found_key==key){void *allocation=NULL;BYTE data[24];
   if(safe_read(node,0x18,&allocation,8)&&safe_read(allocation,0x20,data,sizeof(data))){
    append(line,",\"renamed_allocation\":{\"pointer\":\"%p\",\"cpu_address_hex\":",allocation);hex(line,data,8);
    append(line,",\"gpu_address_hex\":");hex(line,data+8,8);
    append(line,",\"backing_pointer_hex\":");hex(line,data+16,8);
    append(line,",\"provenance\":\"read_only_context_d330_existing_key\"}");return;
   }break;
  }
  if(node==endpoint || !safe_read(node,8,&node,8))break;
 }
 append(line,",\"renamed_allocation\":null");
}
static void upload_metadata(Line *line,BYTE *context,void *device) {
 if(word(context,0xd2c0)){append(line,",\"b1_cpu_last_upload\":null,\"b1_reason\":\"logical_cache_inactive\"");return;}
 void *buffer=pointer(context,0x8a48+(4*14+1)*16);
 if(!candidate_buffer(buffer,device)){append(line,",\"b1_cpu_last_upload\":null,\"b1_reason\":\"not_verified_1664_byte_dynamic_buffer\"");return;}
 UINT id=word(buffer,0x3c);Upload captured={};BOOL found=FALSE;
 renamed_allocation(line,context,buffer);
 AcquireSRWLockShared(&diagnostic_lock);
 for(UINT n=0;n<_countof(uploads);++n)if(uploads[n].valid && uploads[n].resource==buffer &&
    uploads[n].owner==device && uploads[n].id==id){captured=uploads[n];found=TRUE;break;}
 ReleaseSRWLockShared(&diagnostic_lock);
 if(!found){append(line,",\"b1_cpu_last_upload\":null,\"b1_reason\":\"no_completed_observed_upload\"");return;}
 append(line,",\"b1_cpu_last_upload\":{\"resource\":\"%p\",\"resource_id\":%u,\"qpc\":%lld,"
  "\"thread\":%u,\"upload_context\":\"%p\",\"provenance\":\"successful_WRITE_DISCARD_before_Unmap\","
  "\"gpu_readback\":false,\"binding_rename_correspondence_verified\":false,\"bytes\":",
  buffer,id,captured.tick,captured.thread,captured.context);
 hex(line,captured.bytes,sizeof(captured.bytes));
 append(line,",\"mapped_cpu_address\":\"%p\",\"map_epoch\":%u,\"unmap_epoch\":%u",captured.mapped_address,captured.map_epoch,captured.unmap_epoch);
 append(line,",\"buffer_field148_at_upload\":");hex(line,captured.gpu_address,8);
 append(line,",\"backend_b1_address_at_draw\":");hex(line,context+0x5900+(4*14+1)*40+0x18,8);
 append(line,"}");
}
static void extra_pipeline(Line *line,BYTE *context,void *device) {
 append(line,",\"sample_mask\":%u,\"blend_factor_hex\":",word(context,0x6fa0));hex(line,context+0x6f90,16);
 void *blend=pointer(context,0x6f88);append(line,",\"blend_state\":{\"pointer\":\"%p\"",blend);
 if(blend){BOOL same_owner=pointer(blend,0x30)==device;UINT kind=word(blend,0x38);
  BOOL verified=kind==10 && same_owner && !memcmp((BYTE*)blend+0x40,host+0x29fba8,16);
  append(line,",\"kind\":%u,\"owner_matches\":%s,\"description_verified\":%s",kind,same_owner?"true":"false",verified?"true":"false");
  if(verified){append(line,",\"description_hex\":");hex(line,(BYTE*)blend+0x50,264);}
 }
 append(line,"},\"cbv_version_counters\":[%u,%u],\"pixel_samplers\":[",word(context,0xb5d0),word(context,0xb5d4));
 for(UINT n=0;n<16;++n){void *value=pointer(context,0x6820+n*8);
  append(line,"%s{\"slot\":%u,\"pointer\":\"%p\"",n?",":"",n,value);
  if(value && word(value,0x38)==10 && pointer(value,0x30)==device &&
     !memcmp((BYTE*)value+0x40,host+0x29f558,16)){
   append(line,",\"description_hex\":");hex(line,(BYTE*)value+0x1b4,52);
  }else if(value)append(line,",\"unverified_sampler\":true");
  append(line,"}");
 }
 append(line,"],\"output_views\":[");
 for(UINT n=0;n<9;++n){void *view=pointer(context,0x6920+n*8);
  append(line,"%s{\"slot\":%u,\"pointer\":\"%p\"",n?",":"",n,view);
  if(view)view_allocations(line,view,device);
  if(view && word(view,0x38)==11 && pointer(view,0x30)==device &&
     (*(void***)view)[8]==(void*)(host+0x141910)){
   UINT type=((BYTE*)view)[0x75];
   append(line,",\"view_type\":%u",type);
   if(type<4){append(line,",\"description_hex\":");hex(line,(BYTE*)view+0x108,(type==0||type==2)?24:20);}
   append(line,",\"resource\":");resource(line,pointer(view,0x48),device);
  }else if(view)append(line,",\"unverified_view\":true");
  append(line,"}");
 }
 append(line,"]");
 upload_metadata(line,context,device);
}
static void capture_draw(Line *line,BYTE *context,LONGLONG tick,UINT phase,DrawPair *pair) {
 BOOL after=phase==1;
 if(!context || (*(void***)context)[3]!=(void*)(host+0xdbe90) || context[0xc49]!=0)return;
 if(context[0xc48]!=1 && (phase!=2 || context[0xc48]!=0 ||
    (uintptr_t)context!=(uintptr_t)pointer(context,0xc40)+0x6f40))return;
 FM5ColorLock guard={};if(context[0xc48])acquire_lock(&guard,context);
 __try {
  void *device=pointer(context,0xc40),*ps=pointer(context,0x40f0);
  if(!after && (!ps || word(ps,0x38)!=8 || word(ps,0x50)!=4 || pointer(ps,0x30)!=device))return;
  void *program=ps?pointer(ps,0x58):NULL;if(!program&&!after)return;
  BYTE current_key[20]={};if(program)memcpy(current_key,(BYTE*)program+0x168,20);
  TaggedProgram match={};BOOL record=FALSE;
  if(after){match=pair->program;record=pair->active;}
  else {
   AcquireSRWLockExclusive(&diagnostic_lock);
   for(UINT i=0;i<tagged_count;++i)if(!memcmp(tagged[i].key,current_key,20)){
    if(tagged[i].samples<1200 && (!tagged[i].samples || tick-tagged[i].last>=frequency)){
     tagged[i].last=tick;++tagged[i].samples;match=tagged[i];record=TRUE;
    }break;
   }
   ReleaseSRWLockExclusive(&diagnostic_lock);
   if(record){pair->active=TRUE;pair->program=match;pair->id=InterlockedIncrement64(&pair_sequence);}
  }
  if(!record)return;
  append(line,"{\"event\":\"fm5_postprocess_draw\",\"phase\":\"%s\",\"pair_id\":%lld,\"qpc\":%lld,\"qpc_frequency\":%lld,"
   "\"thread\":%lu,\"native_context\":\"%p\",\"device\":\"%p\",\"family\":%u,\"original_shader_rva\":%u,"
   "\"context_lock_held\":%s,\"native_preparation_boundary\":%s,\"gpu_wait\":false,\"observed_key\":",
   phase==2?"after_native_preparation":after?"after_native_draw":"before_native_draw",pair->id,tick,frequency,GetCurrentThreadId(),context,device,match.family,match.rva,
   guard.held?"true":"false",phase==2?"true":"false");
  hex(line,match.key,20);append(line,",\"pixel\":");
  stage(line,context,device,4);
  if(phase==2){append(line,",\"vertex\":");stage(line,context,device,0);}
  extra_pipeline(line,context,device);
  if(phase==2 && match.family==1)actual_bound_payload(line,context,device);
  append(line,"}\n");
 } __finally {release_native_lock(&guard);}
}
static BOOL prepared_program_eligible(BYTE *context,LONGLONG tick) {
 if(!context || (*(void***)context)[3]!=(void*)(host+0xdbe90) || context[0xc49]!=0)return FALSE;
 if(context[0xc48]!=1 && (context[0xc48]!=0 || (uintptr_t)context!=(uintptr_t)pointer(context,0xc40)+0x6f40))return FALSE;
 void *ps=pointer(context,0x40f0);
 if(!ps || word(ps,0x38)!=8 || word(ps,0x50)!=4 || pointer(ps,0x30)!=pointer(context,0xc40))return FALSE;
 void *program=pointer(ps,0x58);if(!program)return FALSE;
 BYTE key[20];memcpy(key,(BYTE*)program+0x168,20);BOOL eligible=FALSE;
 AcquireSRWLockShared(&diagnostic_lock);
 __try {
  for(UINT i=0;i<tagged_count;++i)if(!memcmp(key,tagged[i].key,20)){
   eligible=tagged[i].samples<1200 && (!tagged[i].samples || tick-tagged[i].last>=frequency);break;
  }
 } __finally {ReleaseSRWLockShared(&diagnostic_lock);}
 return eligible;
}
}

extern "C" void FM5ObservePixelShaderCreation(void *shader,const void *data,SIZE_T size) {
 DWORD error=GetLastError();
 if(initialize_enabled())__try {
  UINT rva=0,family=classify(data,size,&rva);
  if(family && shader && word(shader,0x38)==8 && word(shader,0x50)==4){
   void *program=pointer(shader,0x58);if(program){BOOL added=FALSE;BYTE key[20];
    AcquireSRWLockExclusive(&diagnostic_lock);
    __try {
     UINT i=0;for(;i<tagged_count;++i)if(!memcmp(tagged[i].key,(BYTE*)program+0x168,20))break;
     if(i==tagged_count && tagged_count<_countof(tagged)){
      memcpy(tagged[i].key,(BYTE*)program+0x168,20);tagged[i].family=family;tagged[i].rva=rva;++tagged_count;
      memcpy(key,tagged[i].key,20);added=TRUE;
     }
    } __finally {ReleaseSRWLockExclusive(&diagnostic_lock);}
    if(added){char storage[320];Line line={storage,0,sizeof(storage),FALSE};
     append(&line,"{\"event\":\"fm5_postprocess_shader_tag\",\"family\":%u,\"original_rva\":%u,\"original_bytes\":%llu,\"native_key\":",family,rva,(unsigned long long)size);
     hex(&line,key,20);append(&line,"}\n");if(!line.truncated)writer(storage,(DWORD)line.used);
    }
   }
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){}
 SetLastError(error);
}
extern "C" void FM5ObservePostprocessMap(void *context,void *resource,UINT subresource,UINT type,HRESULT result,const void *mapped) {
 DWORD error=GetLastError();
 if(initialize_enabled() && context && resource && !subresource)__try {
  void *device=pointer(context,0xc40);
  if(candidate_buffer(resource,device)){
   UINT id=word(resource,0x3c),map_epoch=word(context,0xb5d0);void *cpu=SUCCEEDED(result)&&type==4&&mapped?pointer(mapped,0):NULL;
   AcquireSRWLockExclusive(&diagnostic_lock);
   UINT i=0;for(;i<_countof(uploads);++i)if(uploads[i].resource==resource)break;
   if(i==_countof(uploads)){i=next_upload++%_countof(uploads);}
   ZeroMemory(&uploads[i],sizeof(uploads[i]));uploads[i].resource=resource;uploads[i].owner=device;
   uploads[i].id=id;uploads[i].mapped=cpu;uploads[i].context=context;uploads[i].thread=GetCurrentThreadId();
   uploads[i].mapped_address=cpu;uploads[i].map_epoch=map_epoch;
   ReleaseSRWLockExclusive(&diagnostic_lock);
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){}
 SetLastError(error);
}
extern "C" void FM5ObservePostprocessUnmap(void *context,void *resource,UINT subresource) {
 DWORD error=GetLastError();
 if(initialize_enabled() && context && resource && !subresource)__try {
  UINT id=word(resource,0x3c);void *device=pointer(context,0xc40);
  AcquireSRWLockExclusive(&diagnostic_lock);
  __try {
   for(UINT n=0;n<_countof(uploads);++n){Upload *entry=&uploads[n];
    if(entry->resource==resource && entry->id==id && entry->owner==device && entry->context==context &&
       entry->thread==GetCurrentThreadId() && entry->mapped){
     __try {memcpy(entry->bytes,entry->mapped,sizeof(entry->bytes));memcpy(entry->gpu_address,(BYTE*)resource+0x148,8);entry->unmap_epoch=word(context,0xb5d0);entry->tick=clock_read();entry->valid=TRUE;}
     __finally {entry->mapped=NULL;}
     break;
    }
   }
  } __finally {ReleaseSRWLockExclusive(&diagnostic_lock);}
 } __except(EXCEPTION_EXECUTE_HANDLER){}
 SetLastError(error);
}
extern "C" void FM5ObservePostprocessDraw(void *context,BOOL after) {
 DWORD error=GetLastError();
 DrawPair *pair;
 if(after){if(!draw_depth){SetLastError(error);return;}--draw_depth;
  if(draw_depth>=_countof(draw_pairs)){SetLastError(error);return;}pair=&draw_pairs[draw_depth];
  if(!pair->active || pair->context!=context){SetLastError(error);return;}
 }else {
  UINT depth=draw_depth++;if(depth>=_countof(draw_pairs)){SetLastError(error);return;}
  pair=&draw_pairs[depth];ZeroMemory(pair,sizeof(*pair));pair->context=context;
 }
 if(initialize_enabled() && context)__try {
  /* Unknown draws and already sampled families allocate no JSON storage. */
  BYTE *native=(BYTE*)context;
  if((*(void***)native)[3]!=(void*)(host+0xdbe90) || native[0xc48]!=1 || native[0xc49]!=0){SetLastError(error);return;}
  void *ps=pointer(native,0x40f0),*program=ps?pointer(ps,0x58):NULL;
  BOOL eligible=FALSE;LONGLONG tick=clock_read();
  if(program && word(ps,0x38)==8 && word(ps,0x50)==4){
   AcquireSRWLockShared(&diagnostic_lock);
   __try {
    for(UINT n=0;n<tagged_count;++n)if(!memcmp(tagged[n].key,(BYTE*)program+0x168,20)){
     eligible=tagged[n].samples<1200 && (!tagged[n].samples || tick-tagged[n].last>=frequency);break;
    }
   } __finally {ReleaseSRWLockShared(&diagnostic_lock);}
  }
  if(!eligible && !after){SetLastError(error);return;}
  char *bytes=(char*)HeapAlloc(GetProcessHeap(),0,65536);
  if(bytes){Line line={bytes,0,65536,FALSE};
   __try {capture_draw(&line,(BYTE*)context,tick,after,pair);if(line.used&&!line.truncated)writer(bytes,(DWORD)line.used);}
   __finally {HeapFree(GetProcessHeap(),0,bytes);}
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){}
 SetLastError(error);
}

extern "C" __declspec(dllexport) void *WINAPI FM5PrepareGraphicsObserved(void *context,BYTE flag,void *opaque) {
#ifndef FM5_COLOR_OBSERVER_TEST
 InitOnceExecuteOnce(&preparation_once,resolve_preparation,NULL,NULL);
#endif
 /* This replaces only the two original CALL12a6d0 sites. The native return
    is an owned preparation pointer, not an HRESULT or a Boolean. */
 if(!original_preparation){RaiseException(0xe05f0062,EXCEPTION_NONCONTINUABLE,0,NULL);return NULL;}
 void *result=original_preparation(context,flag,opaque);
 DWORD error=GetLastError();
#ifndef FM5_COLOR_OBSERVER_TEST
 if(result)FM5ReadbackAfterPreparation(context,result);
#endif
 if(result && initialize_enabled())__try {
  LONGLONG tick=clock_read();
  if(prepared_program_eligible((BYTE*)context,tick)){
   char *bytes=(char*)HeapAlloc(GetProcessHeap(),0,65536);
   if(bytes){Line line={bytes,0,65536,FALSE};DrawPair pair={};pair.context=context;
    __try {capture_draw(&line,(BYTE*)context,tick,2,&pair);if(line.used&&!line.truncated)writer(bytes,(DWORD)line.used);}
    __finally {HeapFree(GetProcessHeap(),0,bytes);}
   }
  }
 } __except(EXCEPTION_EXECUTE_HANDLER){}
 SetLastError(error);return result;
}

extern "C" void WINAPI FM5ObserveColorBeforeUi(void *ui) {
 DWORD saved_error=GetLastError();
#ifndef FM5_COLOR_OBSERVER_TEST
 InitOnceExecuteOnce(&once,initialize,NULL,NULL);
#endif
 if(!enabled || !ui){SetLastError(saved_error);return;}
 __try {
  /* Only the main onscreen UI renderer; offscreen captures are separate. */
  void *main_ui=expected_ui?expected_ui:*(void**)(uintptr_t)0x14261ce38;
  if(ui==main_ui && !*((BYTE*)ui+0x5f0)) {
   LONGLONG now=clock_read();BOOL record=FALSE;
   AcquireSRWLockExclusive(&rate_lock);
   if(snapshots<1200 && (!snapshots || now-last_tick>=frequency)) {
    last_tick=now;++snapshots;record=TRUE;
   }
   ReleaseSRWLockExclusive(&rate_lock);
   if(record) {
    char *bytes=(char*)HeapAlloc(GetProcessHeap(),0,65536);
    if(bytes) {
     Line line={bytes,0,65536,FALSE};
     __try {
      capture(&line,ui,now);
      if(!line.truncated)writer(bytes,(DWORD)line.used);
      else {
       const char message[]="{\"event\":\"fm5_color_observer_unavailable\",\"reason\":\"bounded_json_capacity\"}\n";
       writer(message,(DWORD)(sizeof(message)-1));
      }
     }
     __finally {HeapFree(GetProcessHeap(),0,bytes);}
    }
   }
  }
 } __except(EXCEPTION_EXECUTE_HANDLER) {
  char line[144];int length=_snprintf_s(line,sizeof(line),_TRUNCATE,
   "{\"event\":\"fm5_color_observer_unavailable\",\"code\":\"0x%08lx\"}\n",GetExceptionCode());
  if(length>0)writer(line,(DWORD)length);
 }
 SetLastError(saved_error);
}

#ifdef FM5_COLOR_OBSERVER_TEST
extern "C" void FM5ColorObserverTestConfigure(const BYTE *base,SIZE_T size,void *ui,BOOL collect,
    FM5ColorUnwrap unwrap_callback,FM5ColorClock clock_callback,FM5ColorWrite write_callback,
    FM5ColorAcquire acquire_callback) {
 host=base;expected_ui=ui;frequency=1000;last_tick=0;snapshots=0;
 ZeroMemory(tagged,sizeof(tagged));tagged_count=0;ZeroMemory(uploads,sizeof(uploads));next_upload=0;
 draw_depth=0;ZeroMemory(draw_pairs,sizeof(draw_pairs));pair_sequence=0;
 unwrap=unwrap_callback;clock_read=clock_callback;writer=write_callback;
 acquire_lock=acquire_callback;
 enabled=collect && SUCCEEDED(validate(base,size));
}
extern "C" HRESULT FM5ColorObserverTestGuard(const BYTE *base,SIZE_T size) {return validate(base,size);}
extern "C" UINT FM5ColorObserverTestClassify(const void *bytes,SIZE_T count){return classify(bytes,count,NULL);}
extern "C" void FM5ColorObserverTestPreparation(FM5ColorPreparation prepare){original_preparation=prepare;}
extern "C" BOOL FM5ColorObserverTestViewMetadata(void *view,void *device,char *buffer,UINT capacity){
 Line line={buffer,0,capacity,FALSE};append(&line,"{\"fixture\":true");view_allocations(&line,view,device);append(&line,"}");
 if(line.used<line.capacity)line.bytes[line.used]=0;return !line.truncated;
}
extern "C" BOOL FM5ColorObserverTestActualBound(void *context,void *device,char *buffer,UINT capacity){
 Line line={buffer,0,capacity,FALSE};append(&line,"{\"fixture\":true");actual_bound_payload(&line,(BYTE*)context,device);append(&line,"}");
 if(line.used<line.capacity)line.bytes[line.used]=0;return !line.truncated;
}
#endif
