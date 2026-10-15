#include "fm5_resource.h"
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include "resource_guards.h"
#include "../../allocation-port/fm5_allocation.h"

typedef HRESULT (WINAPI *CreateFn)(void *,const void *,const void *,void **);
typedef HRESULT (WINAPI *PlacementFn)(void *,const void *,UINT,UINT,void *,void **);
typedef void (WINAPI *GetDescFn)(void *,void *);
typedef ULONG (WINAPI *ReleaseFn)(void *);
typedef union NativeCall { void *address; CreateFn create; PlacementFn placement; GetDescFn get; ReleaseFn release; } NativeCall;
typedef struct Entry { void *resource; UINT slot; DWORD extra; DWORD clear_misc_mask; UINT original_sample_count;UINT original_sample_quality;struct Entry *next; } Entry;
static const UINT old_sizes[4]={28,36,48,40};
static const UINT misc_offsets[4]={16,28,40,32};
static SRWLOCK metadata_lock=SRWLOCK_INIT;
static Entry *metadata[256];
static INIT_ONCE placement_trace_once=INIT_ONCE_STATIC_INIT;
static BOOL placement_trace_enabled;
static volatile LONG placement_trace_lines;
static BOOL CALLBACK initialize_placement_trace(PINIT_ONCE once,PVOID parameter,PVOID *context) {
    (void)once;(void)parameter;(void)context;
    WCHAR value[4]={0};
    placement_trace_enabled=GetEnvironmentVariableW(L"FM5_TRACE_PLACEMENT",value,4)==1 && value[0]==L'1';
    return TRUE;
}
static void trace_placement(void *device,const DWORD *description,UINT a,UINT b,void *placement,void **out) {
    if(!InitOnceExecuteOnce(&placement_trace_once,initialize_placement_trace,NULL,NULL) || !placement_trace_enabled)return;
    LONG sequence=InterlockedIncrement(&placement_trace_lines);
    if(sequence>16)return;
    fprintf(stderr,"FM5_PLACEMENT seq=%ld thread=%lu native_device=%p a=0x%x b=0x%x placement=%p out=%p desc13=[%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu,%lu]\n",sequence,GetCurrentThreadId(),device,a,b,placement,(void *)out,description[0],description[1],description[2],description[3],description[4],description[5],description[6],description[7],description[8],description[9],description[10],description[11],description[12]);
    fflush(stderr);
}
#ifdef FM5_RESOURCE_TEST
static volatile LONG fail_allocation=-1;
void FM5ResourceFailAllocation(LONG countdown) { InterlockedExchange(&fail_allocation,countdown); }
SIZE_T FM5ResourceMetadataCount(void) {
    SIZE_T result=0;AcquireSRWLockShared(&metadata_lock);
    for (UINT i=0;i<256;++i) for(Entry *p=metadata[i];p;p=p->next) ++result;
    ReleaseSRWLockShared(&metadata_lock);return result;
}
#endif
static UINT bucket(void *resource) { uintptr_t p=(uintptr_t)resource;return (UINT)((p>>4)^(p>>12))&255; }
static void *allocate(SIZE_T size) {
#ifdef FM5_RESOURCE_TEST
    LONG n=InterlockedCompareExchange(&fail_allocation,-1,-1);
    if(n>=0) { if(!n) { InterlockedExchange(&fail_allocation,-1);return NULL; }InterlockedDecrement(&fail_allocation); }
#endif
    return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,size);
}
static void release_native(void *value) { NativeCall call;call.address=(*(void ***)value)[2];call.release(value); }
static BOOL guard(void *method,UINT index) {
#ifdef FM5_RESOURCE_TEST
    (void)index;return method!=NULL;
#else
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!base || ((IMAGE_DOS_HEADER *)base)->e_magic!=IMAGE_DOS_SIGNATURE) return FALSE;
    LONG nt_offset=((IMAGE_DOS_HEADER *)base)->e_lfanew;
    if(nt_offset<0x40 || nt_offset>0x1000) return FALSE;
    IMAGE_NT_HEADERS64 *pe=(IMAGE_NT_HEADERS64 *)(base+nt_offset);
    DWORD rva=resource_guards[index].rva;
    if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || pe->OptionalHeader.SizeOfImage<rva+16) return FALSE;
    return method==base+rva && memcmp(method,resource_guards[index].bytes,16)==0;
#endif
}
static HRESULT finish_creation(Entry *entry,HRESULT hr,void *resource,void **out) {
    if(FAILED(hr) || !resource) {
        if(resource) release_native(resource);
        HeapFree(GetProcessHeap(),0,entry);
        return FAILED(hr) ? hr : E_UNEXPECTED;
    }
    entry->resource=resource;
    UINT b=bucket(resource);
    AcquireSRWLockExclusive(&metadata_lock);
    Entry *existing=metadata[b];while(existing && existing->resource!=resource) existing=existing->next;
    if(!existing) { entry->next=metadata[b];metadata[b]=entry; }
    ReleaseSRWLockExclusive(&metadata_lock);
    if(existing) { release_native(resource);HeapFree(GetProcessHeap(),0,entry);return HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS); }
    *out=resource;return hr;
}
HRESULT WINAPI FM5CreateLegacyResource(void *native_device,UINT slot,const void *description,const void *initial,void **out) {
    if(out) *out=NULL;
    if(!native_device || !description || slot<3 || slot>6) return E_INVALIDARG;
    UINT index=slot-3,bytes=old_sizes[index];
    DWORD native_description[13]={0},extra=0,misc=0;
    memcpy(native_description,description,bytes);
    memcpy(&extra,(const BYTE *)description+bytes-4,4);
    memcpy(&misc,(const BYTE *)description+misc_offsets[index],4);
    NativeCall call;call.address=(*(void ***)native_device)[slot];
    if(!guard(call.address,index)) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    if(!out) return call.create(native_device,native_description,initial,NULL);
    if((misc&0x20000) && extra==0xffffffff) return E_NOTIMPL;
    Entry *entry=(Entry *)allocate(sizeof(*entry));
    if(!entry) return E_OUTOFMEMORY;
    entry->slot=slot;entry->extra=extra;
    if(slot==5) {memcpy(&entry->original_sample_count,(const BYTE *)description+20,4);memcpy(&entry->original_sample_quality,(const BYTE *)description+24,4);}
    void *resource=NULL;
    HRESULT hr=call.create(native_device,native_description,initial,&resource);
    return finish_creation(entry,hr,resource,out);
}
HRESULT WINAPI FM5CreateLegacyPlacementBuffer(void *native_device,const void *description,void *placement,void **out) {
    if(out)*out=NULL;
    if(!native_device || !description || !out)return E_INVALIDARG;
    Entry *entry=(Entry *)allocate(sizeof(*entry));
    if(!entry)return E_OUTOFMEMORY;
    entry->slot=3;entry->extra=0;entry->clear_misc_mask=0x40000000;
    void *resource=NULL;
    HRESULT hr=FM5CreateNativePlacementBuffer(native_device,description,placement,&resource);
    return finish_creation(entry,hr,resource,out);
}
HRESULT WINAPI FM5CreateLegacyPlacementTexture2D(void *native_device,const void *description,UINT packing_a,UINT packing_b,void *placement,void **out) {
    if(out) *out=NULL;
    if(!native_device || !description) return E_INVALIDARG;
    DWORD native_description[13]={0};memcpy(native_description,description,44);
    NativeCall call;call.address=(*(void ***)native_device)[64];
    if(!guard(call.address,8)) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    if(!out) {trace_placement(native_device,native_description,packing_a,packing_b,placement,NULL);return call.placement(native_device,native_description,packing_a,packing_b,placement,NULL);}
    Entry *entry=(Entry *)allocate(sizeof(*entry));
    if(!entry) return E_OUTOFMEMORY;
    entry->slot=5;entry->extra=(packing_a<<24)|packing_b;entry->clear_misc_mask=0x40000000;
    memcpy(&entry->original_sample_count,(const BYTE *)description+20,4);memcpy(&entry->original_sample_quality,(const BYTE *)description+24,4);
    void *resource=NULL;
    trace_placement(native_device,native_description,packing_a,packing_b,placement,&resource);
    HRESULT hr=call.placement(native_device,native_description,packing_a,packing_b,placement,&resource);
    return finish_creation(entry,hr,resource,out);
}
HRESULT WINAPI FM5CreateLegacyPlacementTexture3D(void *native_device,const void *description,UINT packing_a,UINT packing_b,void *placement,void **out) {
    if(out)*out=NULL;
    if(!native_device || !description)return E_INVALIDARG;
    DWORD native_description[13]={0};memcpy(native_description,description,36);
    NativeCall call;call.address=(*(void ***)native_device)[65];
    if(!guard(call.address,9))return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    if(!out){trace_placement(native_device,native_description,packing_a,packing_b,placement,NULL);return call.placement(native_device,native_description,packing_a,packing_b,placement,NULL);}
    Entry *entry=(Entry *)allocate(sizeof(*entry));
    if(!entry)return E_OUTOFMEMORY;
    entry->slot=6;entry->extra=(packing_a<<24)|packing_b;entry->clear_misc_mask=0x40000000;
    void *resource=NULL;
    trace_placement(native_device,native_description,packing_a,packing_b,placement,&resource);
    HRESULT hr=call.placement(native_device,native_description,packing_a,packing_b,placement,&resource);
    return finish_creation(entry,hr,resource,out);
}
HRESULT WINAPI FM5AdoptLegacyTexture2D(void *native_resource) {
    if(!native_resource)return E_POINTER;
    UINT b=bucket(native_resource);
    AcquireSRWLockShared(&metadata_lock);
    Entry *existing=metadata[b];while(existing && existing->resource!=native_resource)existing=existing->next;
    HRESULT known=existing ? (existing->slot==5 ? S_FALSE : E_NOINTERFACE) : S_OK;
    ReleaseSRWLockShared(&metadata_lock);
    if(known!=S_OK)return known;
    NativeCall call;call.address=(*(void ***)native_resource)[12];
    if(!guard(call.address,6))return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    Entry *entry=(Entry *)allocate(sizeof(*entry));
    if(!entry)return E_OUTOFMEMORY;
    DWORD description[13]={0};call.get(native_resource,description);
    /* A swapchain buffer has no caller-provided ESRAM/placement offset. */
    if(!description[5]) {HeapFree(GetProcessHeap(),0,entry);return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);}
    if(description[10]&0x20000) {HeapFree(GetProcessHeap(),0,entry);return E_NOTIMPL;}
    entry->resource=native_resource;entry->slot=5;entry->extra=0;
    entry->original_sample_count=description[5];entry->original_sample_quality=description[6];
    AcquireSRWLockExclusive(&metadata_lock);
    existing=metadata[b];while(existing && existing->resource!=native_resource)existing=existing->next;
    if(!existing) {entry->next=metadata[b];metadata[b]=entry;}
    known=existing ? (existing->slot==5 ? S_FALSE : E_NOINTERFACE) : S_OK;
    ReleaseSRWLockExclusive(&metadata_lock);
    if(existing)HeapFree(GetProcessHeap(),0,entry);
    return known;
}
static HRESULT get_description(void *native_resource,void *out,SIZE_T capacity,UINT required_slot) {
    if(!native_resource || !out) return E_POINTER;
    Entry snapshot={0};BOOL found=FALSE;
    AcquireSRWLockShared(&metadata_lock);
    for(Entry *p=metadata[bucket(native_resource)];p;p=p->next) if(p->resource==native_resource) {snapshot=*p;found=TRUE;break;}
    ReleaseSRWLockShared(&metadata_lock);
    if(!found) return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    if(required_slot && snapshot.slot!=required_slot)return E_NOINTERFACE;
    UINT index=snapshot.slot-3,bytes=old_sizes[index];
    if(capacity<bytes) return HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
    NativeCall call;call.address=(*(void ***)native_resource)[12];
    if(!guard(call.address,index+4)) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    DWORD local[13]={0};call.get(native_resource,local);
    if(snapshot.slot==5) {local[5]=snapshot.original_sample_count;local[6]=snapshot.original_sample_quality;}
    local[misc_offsets[index]/4]&=~snapshot.clear_misc_mask;
    memcpy((BYTE *)local+bytes-4,&snapshot.extra,4);
    memcpy(out,local,bytes);return S_OK;
}
HRESULT WINAPI FM5MetadataGetDesc(void *native_resource,void *out,SIZE_T capacity) {return get_description(native_resource,out,capacity,0);}
HRESULT WINAPI FM5GetLegacyTexture2DDescription(void *native_resource,void *out48) {return get_description(native_resource,out48,48,5);}
void WINAPI FM5LegacyGetDescription(void *native_resource,void *out) {
    HRESULT hr=FM5MetadataGetDesc(native_resource,out,48);
    if(FAILED(hr)) {
        ULONG_PTR arguments[2]={(ULONG_PTR)native_resource,(ULONG_PTR)(UINT32)hr};
        RaiseException(FM5_E_RESOURCE_METADATA,EXCEPTION_NONCONTINUABLE,2,arguments);
        TerminateProcess(GetCurrentProcess(),ERROR_INVALID_DATA);
    }
}
void WINAPI FM5ForgetLegacyResource(void *native_resource) {
    if(!native_resource) return;
    AcquireSRWLockExclusive(&metadata_lock);
    Entry **link=&metadata[bucket(native_resource)];
    while(*link && (*link)->resource!=native_resource) link=&(*link)->next;
    Entry *entry=*link;if(entry) *link=entry->next;
    ReleaseSRWLockExclusive(&metadata_lock);
    if(entry) HeapFree(GetProcessHeap(),0,entry);
}
