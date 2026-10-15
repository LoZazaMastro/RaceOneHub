#include "fm5_performance_device.h"
#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include "../../graphics-port/fm5_fmask_bridge.h"
#include "fence_guard.h"
#include "../../allocation-port/fm5_msaa4.h"

typedef HRESULT (WINAPI *QueryFn)(void *, REFIID, void **);
typedef ULONG (WINAPI *RefFn)(void *);
typedef void (WINAPI *UnknownFn)(void *, ...);
typedef HRESULT (WINAPI *FeatureFn)(void *, UINT32, UINT32);
typedef HRESULT (WINAPI *CreateFn)(void *, const FM5ScratchDescription *, void **);
typedef void (WINAPI *GetContextFn)(void *, void **);
typedef HRESULT (WINAPI *DeferredFn)(void *, UINT32, void **);
typedef HRESULT (WINAPI *ResourceFn)(void *, const void *, const void *, void **);
typedef HRESULT (WINAPI *PlacementFn)(void *,const void *,UINT,UINT,void *,void **);
typedef HRESULT (WINAPI *PlacementBufferFn)(void *,const void *,void *,void **);
typedef HRESULT (WINAPI *SRVFn)(void *,void *,const void *,void **);
typedef BOOL (WINAPI *FenceFn)(void *,UINT64);
typedef HRESULT (WINAPI *PixelFn)(void *,const void *,SIZE_T,void *,void **);
typedef void (WINAPI *FatalFn)(void *, ULONG_PTR, ULONG_PTR, ULONG_PTR);
typedef union Call { void *address; UnknownFn opaque; GetContextFn get; DeferredFn deferred; ResourceFn resource; FatalFn fatal; PlacementFn placement; PlacementBufferFn placement_buffer; SRVFn srv; FenceFn fence; PixelFn pixel; } Call;
HRESULT WINAPI FM5WrapPerformanceContext(void *native_owned, void **out);
HRESULT WINAPI FM5CreateLegacyResource(void *native_device, UINT slot, const void *description, const void *initial, void **out);
HRESULT WINAPI FM5CreateLegacyPlacementTexture2D(void *native_device,const void *description,UINT packing_a,UINT packing_b,void *placement,void **out);
HRESULT WINAPI FM5CreateLegacyPlacementTexture3D(void *native_device,const void *description,UINT packing_a,UINT packing_b,void *placement,void **out);
HRESULT WINAPI FM5CreateLegacyPlacementBuffer(void *native_device,const void *description,void *placement,void **out);
#include "device_forwarders.h"
typedef struct NativeIUnknown { QueryFn query; RefFn add; RefFn release; } NativeIUnknown;
typedef struct DeviceVtable {
    QueryFn query; RefFn add; RefFn release;
    UnknownFn unavailable[51];
    FeatureFn feature;
    CreateFn create;
    UnknownFn extended[9];
} DeviceVtable;
typedef struct Device Device;
typedef struct Scratch Scratch;
typedef struct Property Property;
typedef struct ScratchVtable {
    QueryFn query; RefFn add; RefFn release;
    void (WINAPI *get_device)(Scratch *, void **);
    HRESULT (WINAPI *get_private)(Scratch *, REFGUID, UINT *, void *);
    HRESULT (WINAPI *set_private)(Scratch *, REFGUID, UINT, const void *);
    HRESULT (WINAPI *set_interface)(Scratch *, REFGUID, void *);
    HRESULT (WINAPI *set_interface2)(Scratch *, REFGUID, void *);
    UINT (WINAPI *get_type)(Scratch *);
} ScratchVtable;
struct Device { const DeviceVtable *vtable; LONG references; void *native; Device *next; };
struct Property { GUID key; UINT bytes; UINT kind; void *value; Property *next; };
struct Scratch {
    const ScratchVtable *vtable;
    LONG references;
    Device *device;
    SRWLOCK property_lock;
    Property *properties;
    FM5ScratchDescription description;
};
C_ASSERT(offsetof(Device,native)==16);
C_ASSERT(offsetof(DeviceVtable,create)==55*8);
C_ASSERT(sizeof(DeviceVtable)==65*8);
C_ASSERT(offsetof(ScratchVtable,get_type)==8*8);

static const GUID iid_unknown = {0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const GUID iid_performance_device = {0x88671610,0x712e,0x4f1e,{0x84,0xab,0x01,0xb5,0x94,0x8b,0xd3,0x73}};
static const GUID iid_device_child = {0x1841e5c8,0x16b0,0x489b,{0xbc,0xc8,0x44,0xcf,0xb0,0xd5,0xde,0xae}};
static const GUID iid_scratch = {0x758ae3bb,0xebec,0x4c81,{0x82,0x7c,0xeb,0x9b,0x7a,0xcd,0x57,0x4a}};
static const GUID iid_device = {0xdb6f6ddb,0xac77,0x4e88,{0x82,0x53,0x81,0x9d,0xf9,0xbb,0xf1,0x40}};
static const GUID iid_device1 = {0xa04bfb29,0x08ef,0x43d6,{0xa4,0x9c,0xa9,0xbd,0xbd,0xcb,0xe6,0x86}};
static const GUID iid_devicex = {0x177700f9,0x876a,0x4436,{0xb3,0x68,0x36,0xa6,0x04,0xf8,0x2c,0xef}};
static SRWLOCK device_lock = SRWLOCK_INIT;
static Device *devices;
static DeviceVtable device_vtable;
static INIT_ONCE device_once = INIT_ONCE_STATIC_INIT;
static ScratchVtable scratch_vtable;

#ifdef FM5_PERFORMANCE_TEST
static volatile LONG fail_allocation = -1;
void FM5PerformanceFailAllocation(LONG countdown) { InterlockedExchange(&fail_allocation, countdown); }
#endif
static void *allocate(SIZE_T size) {
#ifdef FM5_PERFORMANCE_TEST
    LONG value = InterlockedCompareExchange(&fail_allocation, -1, -1);
    if (value >= 0) {
        if (value == 0) { InterlockedExchange(&fail_allocation, -1); return NULL; }
        InterlockedDecrement(&fail_allocation);
    }
#endif
    return HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
}
static void deallocate(void *value) { if (value) HeapFree(GetProcessHeap(), 0, value); }
static BOOL equal_guid(REFGUID a, REFGUID b) { return a && b && memcmp(a,b,sizeof(GUID)) == 0; }
static NativeIUnknown *native_table(void *value) { return *(NativeIUnknown **)value; }

static ULONG WINAPI device_add(void *self) {
    Device *device = (Device *)self;
    AcquireSRWLockExclusive(&device_lock);
    ULONG value = (ULONG)++device->references;
    ReleaseSRWLockExclusive(&device_lock);
    return value;
}
static ULONG WINAPI device_release(void *self) {
    Device *device = (Device *)self;
    AcquireSRWLockExclusive(&device_lock);
    ULONG value = (ULONG)--device->references;
    if (!value) {
        Device **link = &devices;
        while (*link && *link != device) link = &(*link)->next;
        if (*link) *link = device->next;
    }
    ReleaseSRWLockExclusive(&device_lock);
    if (!value) { native_table(device->native)->release(device->native); deallocate(device); }
    return value;
}
static HRESULT WINAPI device_query(void *self, REFIID iid, void **out) {
    if (!out) return E_POINTER;
    *out = NULL;
    if (!iid) return E_INVALIDARG;
    if (equal_guid(iid,&iid_unknown) || equal_guid(iid,&iid_performance_device)) {
        device_add(self); *out = self; return S_OK;
    }
    Device *device = (Device *)self;
    HRESULT hr = native_table(device->native)->query(device->native,iid,out);
    if (SUCCEEDED(hr) && *out && (equal_guid(iid,&iid_device) || equal_guid(iid,&iid_device1) || equal_guid(iid,&iid_devicex))) {
        void *owned = *out;
        hr = FM5WrapPerformanceDevice(owned,out);
        if (FAILED(hr)) { native_table(owned)->release(owned); *out = NULL; }
    }
    return hr;
}
static volatile LONG fatal_logged[65];
__declspec(noreturn) static void unsupported(Device *device,UINT slot,ULONG_PTR a,ULONG_PTR b,ULONG_PTR c) {
    if(slot<65 && !InterlockedExchange(&fatal_logged[slot],1)) {
        fprintf(stderr,"FM5_DEVICE_UNSUPPORTED old_slot=%u native=%p thread=%lu arg1=0x%llx arg2=0x%llx arg3=0x%llx\n",slot,device->native,GetCurrentThreadId(),(unsigned long long)a,(unsigned long long)b,(unsigned long long)c);
        fflush(stderr);
    }
    RaiseException(FM5_E_UNMAPPED_DEVICE_SLOT, EXCEPTION_NONCONTINUABLE, 0, NULL);
    TerminateProcess(GetCurrentProcess(), ERROR_NOT_SUPPORTED);
    __assume(0);
}
#define UNKNOWN_SLOT(n) static void WINAPI unsupported##n(void *self,ULONG_PTR a,ULONG_PTR b,ULONG_PTR c) {unsupported((Device *)self,n,a,b,c);}
UNKNOWN_SLOT(56) UNKNOWN_SLOT(57) UNKNOWN_SLOT(58) UNKNOWN_SLOT(59) UNKNOWN_SLOT(60)
UNKNOWN_SLOT(61) UNKNOWN_SLOT(62) UNKNOWN_SLOT(63) UNKNOWN_SLOT(64)
static HRESULT WINAPI feature(void *self, UINT32 mode, UINT32 value) {
    if (mode == 4) return E_NOTIMPL;
    unsupported((Device *)self,54,mode,value,0);
}
static HRESULT WINAPI resource3(void *self, const void *description, const void *initial, void **out) { return FM5CreateLegacyResource(((Device *)self)->native,3,description,initial,out); }
static HRESULT WINAPI resource4(void *self, const void *description, const void *initial, void **out) { return FM5CreateLegacyResource(((Device *)self)->native,4,description,initial,out); }
static HRESULT WINAPI resource5(void *self, const void *description, const void *initial, void **out) { return FM5CreateLegacyResource(((Device *)self)->native,5,description,initial,out); }
static HRESULT WINAPI resource6(void *self, const void *description, const void *initial, void **out) { return FM5CreateLegacyResource(((Device *)self)->native,6,description,initial,out); }
static HRESULT WINAPI placement60(void *self,const void *description,UINT packing_a,UINT packing_b,void *placement,void **out) {return FM5CreateLegacyPlacementTexture2D(((Device *)self)->native,description,packing_a,packing_b,placement,out);}
static HRESULT WINAPI placement61(void *self,const void *description,UINT packing_a,UINT packing_b,void *placement,void **out) {return FM5CreateLegacyPlacementTexture3D(((Device *)self)->native,description,packing_a,packing_b,placement,out);}
static HRESULT WINAPI placement58(void *self,const void *description,void *placement,void **out) {return FM5CreateLegacyPlacementBuffer(((Device *)self)->native,description,placement,out);}
static HRESULT WINAPI resource7(void *self,void *resource,const void *description,void **out) {return FM5CreateLegacyShaderResourceView(((Device *)self)->native,resource,description,out);}
static HRESULT WINAPI pixel15(void *self,const void *bytecode,SIZE_T bytes,void *class_linkage,void **out) {return FM5CreateLegacyPixelShader(((Device *)self)->native,bytecode,bytes,class_linkage,out);}
static BOOL fence_guard(void *method) {
#ifdef FM5_PERFORMANCE_TEST
    return method!=NULL;
#else
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!base || ((IMAGE_DOS_HEADER *)base)->e_magic!=IMAGE_DOS_SIGNATURE)return FALSE;
    LONG nt_offset=((IMAGE_DOS_HEADER *)base)->e_lfanew;
    if(nt_offset<0x40 || nt_offset>0x1000)return FALSE;
    IMAGE_NT_HEADERS64 *pe=(IMAGE_NT_HEADERS64 *)(base+nt_offset);
    if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || pe->OptionalHeader.SizeOfImage<0xc3a00)return FALSE;
    return method==base+0xc39f0 && !memcmp(method,native_fence_guard,16);
#endif
}
static BOOL WINAPI fence56(void *self,UINT64 fence) {
    Device *device=(Device *)self;
    Call call;call.address=(*(void ***)device->native)[60];
    if(!fence_guard(call.address))unsupported(device,56,(ULONG_PTR)fence,ERROR_BAD_EXE_FORMAT,0);
    return call.fence(device->native,fence)!=FALSE;
}
static void get_context(Device *device, void **out, unsigned slot) {
    if (!out) return;
    *out = NULL;
    void *native = NULL;
    Call call; call.address = (*(void ***)device->native)[slot];
    call.get(device->native,&native);
    if (!native) unsupported(device,slot,(ULONG_PTR)out,(ULONG_PTR)(UINT32)E_UNEXPECTED,0);
    HRESULT hr = FM5WrapPerformanceContext(native,out);
    if (FAILED(hr)) { native_table(native)->release(native); *out = NULL; unsupported(device,slot,(ULONG_PTR)out,(ULONG_PTR)(UINT32)hr,0); }
}
static void WINAPI get_context41(void *self, void **out) { get_context((Device *)self,out,41); }
static void WINAPI get_context44(void *self, void **out) { get_context((Device *)self,out,44); }
static void WINAPI get_context51(void *self, void **out) { get_context((Device *)self,out,51); }
static HRESULT deferred(Device *device, UINT32 flags, void **out, unsigned slot) {
    if (!out) return E_POINTER;
    *out = NULL;
    void *native = NULL;
    Call call; call.address = (*(void ***)device->native)[slot];
    HRESULT hr = call.deferred(device->native,flags,&native);
    if (FAILED(hr)) { if (native) native_table(native)->release(native); return hr; }
    if (!native) return E_FAIL;
    hr = FM5WrapPerformanceContext(native,out);
    if (FAILED(hr)) { native_table(native)->release(native); *out = NULL; }
    return hr;
}
static HRESULT WINAPI deferred27(void *self, UINT32 flags, void **out) { return deferred((Device *)self,flags,out,27); }
static HRESULT WINAPI deferred45(void *self, UINT32 flags, void **out) { return deferred((Device *)self,flags,out,45); }
static HRESULT WINAPI deferred52(void *self, UINT32 flags, void **out) { return deferred((Device *)self,flags,out,52); }
static void property_destroy(Property *entry) {
    if (entry->kind) native_table(entry->value)->release(entry->value);
    else deallocate(entry->value);
    deallocate(entry);
}
static HRESULT set_property(Scratch *scratch, REFGUID key, UINT bytes, const void *data, UINT kind) {
    if (!key) return E_INVALIDARG;
    Property *entry = NULL;
    if (data) {
        entry = (Property *)allocate(sizeof(*entry));
        if (!entry) return E_OUTOFMEMORY;
        entry->key = *key; entry->bytes = bytes; entry->kind = kind;
        if (kind) {
            entry->value = (void *)data;
            native_table(entry->value)->add(entry->value);
        } else {
            entry->value = bytes ? allocate(bytes) : NULL;
            if (bytes && !entry->value) { deallocate(entry); return E_OUTOFMEMORY; }
            if (bytes) memcpy(entry->value,data,bytes);
        }
    }
    AcquireSRWLockExclusive(&scratch->property_lock);
    Property **link = &scratch->properties;
    while (*link && !equal_guid(&(*link)->key,key)) link = &(*link)->next;
    Property *old = *link;
    if (entry) { entry->next = old ? old->next : NULL; *link = entry; }
    else if (old) *link = old->next;
    ReleaseSRWLockExclusive(&scratch->property_lock);
    if (old) property_destroy(old);
    return entry || old ? S_OK : (HRESULT)0x88790002;
}
static ULONG WINAPI scratch_add(void *self) { return (ULONG)InterlockedIncrement(&((Scratch *)self)->references); }
static ULONG WINAPI scratch_release(void *self) {
    Scratch *scratch = (Scratch *)self;
    ULONG value = (ULONG)InterlockedDecrement(&scratch->references);
    if (!value) {
        Property *entry = scratch->properties;
        while (entry) { Property *next = entry->next; property_destroy(entry); entry = next; }
        device_release(scratch->device); deallocate(scratch);
    }
    return value;
}
static HRESULT WINAPI scratch_query(void *self, REFIID iid, void **out) {
    if (!out) return E_POINTER;
    *out = NULL;
    if (!iid) return E_INVALIDARG;
    if (!equal_guid(iid,&iid_unknown) && !equal_guid(iid,&iid_device_child) && !equal_guid(iid,&iid_scratch)) return E_NOINTERFACE;
    scratch_add(self); *out = self; return S_OK;
}
static void WINAPI get_device(Scratch *scratch, void **out) {
    if (out) { device_add(scratch->device); *out = scratch->device; }
}
static HRESULT WINAPI get_private(Scratch *scratch, REFGUID key, UINT *bytes, void *data) {
    if (!key || !bytes) return E_INVALIDARG;
    AcquireSRWLockShared(&scratch->property_lock);
    Property *entry = scratch->properties;
    while (entry && !equal_guid(&entry->key,key)) entry = entry->next;
    HRESULT hr = S_OK;
    if (!entry) { *bytes = 0; hr = (HRESULT)0x88790002; }
    else {
        UINT capacity = *bytes; *bytes = entry->bytes;
        if (entry->kind) {
            native_table(entry->value)->add(entry->value);
            if (data && capacity >= sizeof(void *)) memcpy(data,&entry->value,sizeof(void *));
        } else if (data && capacity) memcpy(data,entry->value,capacity < entry->bytes ? capacity : entry->bytes);
    }
    ReleaseSRWLockShared(&scratch->property_lock);
    return hr;
}
static HRESULT WINAPI set_private(Scratch *scratch, REFGUID key, UINT bytes, const void *data) { return set_property(scratch,key,bytes,data,0); }
static HRESULT WINAPI set_interface(Scratch *scratch, REFGUID key, void *value) { return set_property(scratch,key,value ? sizeof(void *) : 0,value,1); }
static HRESULT WINAPI set_interface2(Scratch *scratch, REFGUID key, void *value) { return set_property(scratch,key,value ? sizeof(void *) : 0,value,2); }
static UINT WINAPI get_type(Scratch *scratch) { (void)scratch; return 2; }
static ScratchVtable scratch_vtable = {scratch_query,scratch_add,scratch_release,get_device,get_private,set_private,set_interface,set_interface2,get_type};

static HRESULT WINAPI create(void *self, const FM5ScratchDescription *description, void **out) {
    if (!out) return E_POINTER;
    *out = NULL;
    if (!description || description->flags != 1 || description->bytes != 65536) return E_INVALIDARG;
    Scratch *scratch = (Scratch *)allocate(sizeof(*scratch));
    if (!scratch) return E_OUTOFMEMORY;
    scratch->vtable = &scratch_vtable; scratch->references = 1;
    scratch->device = (Device *)self; scratch->description = *description;
    InitializeSRWLock(&scratch->property_lock);
    device_add(self); *out = scratch; return S_OK;
}
static BOOL CALLBACK initialize(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    (void)once; (void)parameter; (void)context;
    device_vtable.query = device_query; device_vtable.add = device_add; device_vtable.release = device_release;
    for (unsigned i = 0; i < 51; ++i) device_vtable.unavailable[i] = device_forwarders[i];
    Call call;
    FatalFn fatal[9]={unsupported56,unsupported57,unsupported58,unsupported59,unsupported60,unsupported61,unsupported62,unsupported63,unsupported64};
    for(unsigned i=0;i<9;++i) {call.fatal=fatal[i];device_vtable.extended[i]=call.opaque;}
    call.fence=fence56;device_vtable.extended[56-56]=call.opaque;
    call.placement_buffer=placement58;device_vtable.extended[58-56]=call.opaque;
    call.placement=placement60;device_vtable.extended[60-56]=call.opaque;
    call.placement=placement61;device_vtable.extended[61-56]=call.opaque;
    call.resource = resource3; device_vtable.unavailable[0] = call.opaque;
    call.resource = resource4; device_vtable.unavailable[1] = call.opaque;
    call.resource = resource5; device_vtable.unavailable[2] = call.opaque;
    call.resource = resource6; device_vtable.unavailable[3] = call.opaque;
    call.srv=resource7;device_vtable.unavailable[7-3]=call.opaque;
    call.pixel=pixel15;device_vtable.unavailable[15-3]=call.opaque;
    call.get = get_context41; device_vtable.unavailable[41-3] = call.opaque;
    call.get = get_context44; device_vtable.unavailable[44-3] = call.opaque;
    call.get = get_context51; device_vtable.unavailable[51-3] = call.opaque;
    call.deferred = deferred27; device_vtable.unavailable[27-3] = call.opaque;
    call.deferred = deferred45; device_vtable.unavailable[45-3] = call.opaque;
    call.deferred = deferred52; device_vtable.unavailable[52-3] = call.opaque;
    device_vtable.feature = feature; device_vtable.create = create;
    return TRUE;
}
HRESULT WINAPI FM5WrapPerformanceDevice(void *native_owned, void **out) {
    if (!out) return E_POINTER;
    *out = NULL;
    if (!native_owned) return E_INVALIDARG;
    if (!InitOnceExecuteOnce(&device_once,initialize,NULL,NULL)) return HRESULT_FROM_WIN32(GetLastError());
    AcquireSRWLockExclusive(&device_lock);
    Device *device = devices;
    while (device && device != native_owned) device = device->next;
    if (device) { *out = device; ReleaseSRWLockExclusive(&device_lock); return S_OK; }
    device = devices;
    while (device && device->native != native_owned) device = device->next;
    if (device) {
        ++device->references; *out = device;
        ReleaseSRWLockExclusive(&device_lock);
        native_table(native_owned)->release(native_owned);
        return S_OK;
    }
    device = (Device *)allocate(sizeof(*device));
    if (!device) { ReleaseSRWLockExclusive(&device_lock); return E_OUTOFMEMORY; }
    device->vtable = &device_vtable; device->references = 1; device->native = native_owned;
    device->next = devices; devices = device; *out = device;
    ReleaseSRWLockExclusive(&device_lock);
    return S_OK;
}
void *WINAPI FM5UnwrapPerformanceDevice(void *candidate) {
    AcquireSRWLockShared(&device_lock);
    Device *device = devices;
    while (device && device != candidate) device = device->next;
    void *native = device ? device->native : candidate;
    ReleaseSRWLockShared(&device_lock);
    return native;
}
BOOL WINAPI FM5InspectScratch(void *candidate, FM5ScratchDescription *out) {
    if (!candidate || !out || *(const void **)candidate != &scratch_vtable) return FALSE;
    *out = ((Scratch *)candidate)->description;
    return TRUE;
}
