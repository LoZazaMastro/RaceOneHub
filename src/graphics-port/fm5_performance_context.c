#include "fm5_performance_context.h"
#include <stddef.h>
#include <stdio.h>
#define CINTERFACE
#pragma warning(push)
#pragma warning(disable: 4201) /* Windows SDK anonymous ABI unions. */
#include <d3d11_1.h>
#pragma warning(pop)

typedef HRESULT (*NativeQuery)(void *, const GUID *, void **);
typedef ULONG (*NativeRelease)(void *);
typedef struct PerformanceContext PerformanceContext;

struct PerformanceContext {
    void **vtable;
    LONG references;
    DWORD reserved;
    void *native;
    DWORD automatic_completion;
    DWORD reserved1;
    void *inline_methods[200];
    PerformanceContext *next;
};

typedef char check_inline_offset[(offsetof(PerformanceContext, inline_methods) == 0x20) ? 1 : -1];
static SRWLOCK cache_lock = SRWLOCK_INIT;
static PerformanceContext *contexts;
static FM5AutoCompletionPolicy completion_policy;
static FM5TopologyPolicy topology_policy;
static FM5DeviceWrapper device_wrapper_policy;
static FM5Texture2DDescriptionPolicy texture_description_policy;
static FM5ScissorSnapshotPolicy scissor_snapshot_policy;
static FM5PixelSamplersPolicy pixel_samplers_policy;
static FM5ContextDrawObserver draw_observer;
static FM5ContextMapObserver map_observer;
static FM5ContextUnmapObserver unmap_observer;
void FM5ConfigureContextDrawObserver(FM5ContextDrawObserver observer) { draw_observer=observer; }
void FM5ConfigureContextMapObserver(FM5ContextMapObserver mapped,FM5ContextUnmapObserver unmapping) {
    map_observer=mapped;unmap_observer=unmapping;
}
static const GUID performance_iid = {0x9458fe06,0xc78d,0x47f7,{0x96,0xa0,0xec,0x7b,0x72,0x7b,0xe1,0xe9}};
static const GUID unknown_iid = {0,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static const GUID context_iid = {0xc0bfa96c,0xe089,0x44fb,{0x8e,0xaf,0x26,0xf8,0x79,0x61,0x90,0xda}};
static const GUID context1_iid = {0xbb2c6faa,0xb5fb,0x4082,{0x8e,0x6b,0x38,0x8b,0x8c,0xfa,0x90,0xe1}};
static const GUID contextx_iid = {0x48800095,0x7134,0x4be7,{0x91,0x86,0xb8,0x6b,0xec,0xb2,0x64,0x77}};
static const GUID child_iid = {0x1841e5c8,0x16b0,0x489b,{0xbc,0xc8,0x44,0xcf,0xb0,0xd5,0xde,0xae}};

static int same_guid(const GUID *a, const GUID *b) {
    const unsigned char *x = (const unsigned char *)a, *y = (const unsigned char *)b;
    unsigned i;
    for (i = 0; i < sizeof(GUID); ++i) if (x[i] != y[i]) return 0;
    return 1;
}

static ULONG release_native(void *object) {
    return ((NativeRelease)(*(void ***)object)[2])(object);
}

static ULONG add_ref(PerformanceContext *context) {
    return (ULONG)InterlockedIncrement(&context->references);
}

static HRESULT query(PerformanceContext *context, const GUID *iid, void **result) {
    void *owned = NULL;
    HRESULT status;
    if (!result) return E_POINTER;
    *result = NULL;
    if (!iid) return E_INVALIDARG;
    if (same_guid(iid, &performance_iid) || same_guid(iid, &unknown_iid)) {
        add_ref(context);
        *result = context;
        return S_OK;
    }
    status = ((NativeQuery)(*(void ***)context->native)[0])(context->native, iid, &owned);
    if (FAILED(status)) return status;
    if (!owned) return E_UNEXPECTED;
    if (same_guid(iid, &context_iid) || same_guid(iid, &context1_iid) ||
        same_guid(iid, &contextx_iid) || same_guid(iid, &child_iid)) {
        status = FM5WrapPerformanceContext(owned, result);
        if (FAILED(status)) release_native(owned);
        return status;
    }
    *result = owned;
    return status;
}

static ULONG release(PerformanceContext *context) {
    PerformanceContext **link;
    LONG remaining;
    AcquireSRWLockExclusive(&cache_lock);
    remaining = InterlockedDecrement(&context->references);
    if (!remaining) {
        for (link = &contexts; *link; link = &(*link)->next) {
            if (*link == context) { *link = context->next; break; }
        }
    }
    ReleaseSRWLockExclusive(&cache_lock);
    if (!remaining) {
        release_native(context->native);
        HeapFree(GetProcessHeap(), 0, context);
    }
    return (ULONG)remaining;
}

__declspec(noreturn) static void unsupported(PerformanceContext *context, ULONG slot) {
    ULONG_PTR information[2] = {(ULONG_PTR)context->native, slot};
    char diagnostic[160];
    DWORD written;
    int count = _snprintf_s(diagnostic, sizeof(diagnostic), _TRUNCATE,
        "FM5 unsupported Context slot=0x%lx native=%p proxy=%p\r\n",
        slot, context->native, (void *)context);
    if (count > 0) WriteFile(GetStdHandle(STD_ERROR_HANDLE), diagnostic,
        (DWORD)count, &written, NULL);
    RaiseException(0xe0f50006, EXCEPTION_NONCONTINUABLE, 2, information);
    TerminateProcess(GetCurrentProcess(), 0xe0f50006);
    __assume(0);
}

#define FM5_PERF_SLOT(index) \
    static void unsupported_##index(PerformanceContext *context) { unsupported(context, index); }
#include "fm5_perf_slots.inc"
#undef FM5_PERF_SLOT

#include "fm5_context_standard_forwarders.inc"

static HRESULT observed_map(PerformanceContext *context,ID3D11Resource *resource,UINT subresource,
                           D3D11_MAP type,UINT flags,D3D11_MAPPED_SUBRESOURCE *mapped) {
    HRESULT result=forward_15(context,resource,subresource,type,flags,mapped);
    if(map_observer)map_observer(context->native,resource,subresource,(UINT)type,result,mapped);
    return result;
}
static void observed_unmap(PerformanceContext *context,ID3D11Resource *resource,UINT subresource) {
    if(unmap_observer)unmap_observer(context->native,resource,subresource);
    forward_16(context,resource,subresource);
}

static void get_device(PerformanceContext *context, ID3D11Device **result) {
    void *owned = NULL;
    FM5DeviceWrapper wrapper;
    HRESULT status;
    if (!result) unsupported(context, 3);
    *result = NULL;
    ((void (*)(void *, void **))(*(void ***)context->native)[3])(context->native, &owned);
    if (!owned) unsupported(context, 3);
    AcquireSRWLockShared(&cache_lock);
    wrapper = device_wrapper_policy;
    ReleaseSRWLockShared(&cache_lock);
    if (!wrapper) { release_native(owned); unsupported(context, 3); }
    status = wrapper(owned, (void **)result);
    if (FAILED(status)) { release_native(owned); unsupported(context, 3); }
}

static void set_topology(PerformanceContext *context, D3D11_PRIMITIVE_TOPOLOGY topology) {
    FM5TopologyPolicy policy;
    AcquireSRWLockShared(&cache_lock);
    policy = topology_policy;
    ReleaseSRWLockShared(&cache_lock);
    if (!policy || FAILED(policy(context->native, (UINT)topology))) unsupported(context, 0x19);
}

static void set_index_buffer(PerformanceContext *context, ID3D11Buffer *buffer, DXGI_FORMAT format, UINT offset) {
    UINT xbox_format;
    /* Old UMD7798/b9fc accepts DXGI formats; Az1b3640 requires Xbox0/1.
     * Az1b3720 converts0 back to R16_UINT and1 back to R32_UINT. The old
     * methods do not inspect Format when no buffer is bound. */
    if (!buffer || format == DXGI_FORMAT_R16_UINT) xbox_format = 0;
    else if (format == DXGI_FORMAT_R32_UINT) xbox_format = 1;
    else unsupported(context, 0x14);
    ((void (*)(void *, UINT, void *, UINT))(*(void ***)context->native)[0x14])
        (context->native, xbox_format, buffer, offset);
}

static void resolve_subresource(PerformanceContext *context, void *destination, UINT destination_subresource,
                               void *source, UINT source_subresource, DXGI_FORMAT format) {
    FM5Texture2DDescriptionPolicy description;
    UINT source_desc[12], destination_desc[12];
    AcquireSRWLockShared(&cache_lock);
    description = texture_description_policy;
    ReleaseSRWLockShared(&cache_lock);
    /* Original UMD84a4 reads the requested SampleDesc.Count encoded by391ec.
     * Count1 branches to1ffcc (full CopySubresourceRegion), ignoring Format;
     * Count>1 tail-calls20340 (Resolve). Keep Az's own MSAA override handling.
     * Never invoke a Texture2D getter on an unverified resource kind. */
    if (!description || !source || !destination ||
        FAILED(description(source, source_desc)) ||
        FAILED(description(destination, destination_desc)) || !source_desc[5])
        unsupported(context, 0x3a);
    if (source_desc[5] == 1) {
        ((void (*)(void *, void *, UINT, UINT, UINT, UINT, void *, UINT, const D3D11_BOX *))
            (*(void ***)context->native)[0x2e])
            (context->native, destination, destination_subresource, 0, 0, 0,
             source, source_subresource, NULL);
    } else {
        ((void (*)(void *, void *, UINT, void *, UINT, DXGI_FORMAT))
            (*(void ***)context->native)[0x39])
            (context->native, destination, destination_subresource, source, source_subresource, format);
    }
}

static void get_scissor_rectangles(PerformanceContext *context, UINT *count, D3D11_RECT *rectangles) {
    FM5ScissorSnapshotPolicy snapshot;
    D3D11_RECT actual[16];
    UINT bound_count, copied;
    if (!count) unsupported(context, 0x61);
    AcquireSRWLockShared(&cache_lock);
    snapshot = scissor_snapshot_policy;
    ReleaseSRWLockShared(&cache_lock);
    if (!snapshot || FAILED(snapshot(context->native, &bound_count, actual)) || bound_count > 16)
        unsupported(context, 0x61);
    /* Original UMD9790->6a5c: NULL array queries actual count. Otherwise it
     * returns min(capacity,bound), copying that prefix and zeroing unused
     * caller entries. The snapshot is authoritative native state, not a
     * setter mirror that could miss ClearState/command-list restore. */
    if (!rectangles) { *count = bound_count; return; }
    copied = *count < bound_count ? *count : bound_count;
    if (copied) CopyMemory(rectangles, actual, (SIZE_T)copied * sizeof(*rectangles));
    if (*count > copied) ZeroMemory(rectangles + copied, (SIZE_T)(*count - copied) * sizeof(*rectangles));
    *count = copied;
}

static void get_pixel_samplers(PerformanceContext *context,UINT start,UINT count,ID3D11SamplerState **samplers) {
    FM5PixelSamplersPolicy getter;
    /* Original UMD9190/120d4 performs no reads or AddRefs for a NULL array. */
    if(!samplers)return;
    AcquireSRWLockShared(&cache_lock);
    getter=pixel_samplers_policy;
    ReleaseSRWLockShared(&cache_lock);
    if(!getter || FAILED(getter(context->native,start,count,(void**)samplers)))
        unsupported(context,0x4c);
}

/* New SDK packs StartIndexLocation/IndexCount and BaseVertex/StartInstance.
 * Az 12b940/12bae0 decode these into D3D12 DrawIndexedInstanced (edf00).
 * Cast through UINT preserves negative BaseVertex as its 32-bit bit pattern. */
static void draw(PerformanceContext *context, UINT count, UINT start) {
    if(draw_observer)draw_observer(context->native,FALSE);
    ((void (*)(void *, UINT, UINT))(*(void ***)context->native)[0x0e])(context->native,count,start);
    if(draw_observer)draw_observer(context->native,TRUE);
}

static void draw_indexed(PerformanceContext *context, UINT count, UINT start, INT base_vertex) {
    if(draw_observer)draw_observer(context->native,FALSE);
    UINT64 packed = ((UINT64)count << 32) | start;
    ((void (*)(void *, UINT64, INT))(*(void ***)context->native)[0x0d])
        (context->native, packed, base_vertex);
    if(draw_observer)draw_observer(context->native,TRUE);
}

static void draw_indexed_instanced(PerformanceContext *context, UINT count, UINT instances,
                                  UINT start, INT base_vertex, UINT first_instance) {
    if(draw_observer)draw_observer(context->native,FALSE);
    UINT64 indices = ((UINT64)count << 32) | start;
    UINT64 vertices = ((UINT64)first_instance << 32) | (UINT)base_vertex;
    ((void (*)(void *, UINT64, UINT64, UINT))(*(void ***)context->native)[0x15])
        (context->native, indices, vertices, instances);
    if(draw_observer)draw_observer(context->native,TRUE);
}

static void draw_instanced(PerformanceContext *context, UINT count, UINT instances,
                          UINT start, UINT first_instance) {
    if(draw_observer)draw_observer(context->native,FALSE);
    UINT64 vertices = ((UINT64)first_instance << 32) | start;
    ((void (*)(void *, UINT, UINT64, UINT))(*(void ***)context->native)[0x16])
        (context->native, count, vertices, instances);
    if(draw_observer)draw_observer(context->native,TRUE);
}

static void single_vertex_buffer(PerformanceContext *context, UINT slot, void *buffer, UINT stride) {
    void *one = buffer;
    UINT offset = 0;
    ((void (*)(void *, UINT, UINT, void *const *, const UINT *, const UINT *))
        (*(void ***)context->native)[0x13])(context->native, slot, 1, &one, &stride, &offset);
}

static void wait_resource(PerformanceContext *context, void *resource) {
    ((void (*)(void *, void *))(*(void ***)context->native)[0x97])(context->native, resource);
}

static void flush_caches(PerformanceContext *context, UINT flags, UINT64 address, UINT64 bytes) {
    ((void (*)(void *, UINT, UINT64, UINT64))(*(void ***)context->native)[0x98])
        (context->native, flags, address, bytes);
}

static void wait_idle(PerformanceContext *context, UINT flags) {
    ((void (*)(void *, UINT))(*(void ***)context->native)[0x99])(context->native, flags);
}

static UINT64 insert_fence(PerformanceContext *context, UINT flags) {
    return ((UINT64 (*)(void *, UINT))(*(void ***)context->native)[0x9a])(context->native, flags);
}

static void wait_fence(PerformanceContext *context, UINT flags, UINT64 fence) {
    ((void (*)(void *, UINT, UINT64))(*(void ***)context->native)[0x9b])(context->native, flags, fence);
}

static void inheritance(PerformanceContext *context, unsigned target,
                        UINT to_stage, UINT to_slot, UINT from_stage, UINT from_slot) {
    UINT type = ((UINT (*)(void *))(*(void ***)context->native)[0x70])(context->native);
    /* Original FM5 immediate table entries are literal RET 0. Only deferred
     * contexts use the remap methods at UMD10ae0/10b0c. Preserve that split. */
    if (type == D3D11_DEVICE_CONTEXT_IMMEDIATE) return;
    if (type != D3D11_DEVICE_CONTEXT_DEFERRED) unsupported(context, target - 5);
    ((void (*)(void *, UINT, UINT, UINT, UINT))(*(void ***)context->native)[target])
        (context->native, to_stage, to_slot, from_stage, from_slot);
}

static void inherit_cb(PerformanceContext *context, UINT a, UINT b, UINT c, UINT d) {
    inheritance(context, 0x9c, a, b, c, d);
}

static void inherit_srv(PerformanceContext *context, UINT a, UINT b, UINT c, UINT d) {
    inheritance(context, 0x9d, a, b, c, d);
}

/* Legacy single-binding methods emit the same register ranges as the array
 * setters. Feed one object to Az's existing setter, preserving its resource
 * ownership, deferred recording, and hazard tracking instead of copying SRDs. */
#define SINGLE_BIND(old_index, az_index) \
    static void single_##old_index(PerformanceContext *context, UINT slot, void *object) { \
        void *one = object; \
        ((void (*)(void *, UINT, UINT, void *const *))(*(void ***)context->native)[az_index]) \
            (context->native, slot, 1, &one); \
    }
SINGLE_BIND(155, 0x11) /* PS CB: hardware register 0x6a0 */
SINGLE_BIND(156, 0x09) /* PS SRV: 0x35a */
SINGLE_BIND(157, 0x0b) /* PS sampler: 0x7b4 */
SINGLE_BIND(158, 0x08) /* VS CB: 0x1c4 */
SINGLE_BIND(159, 0x19) /* VS SRV: 0xec */
SINGLE_BIND(160, 0x1a) /* VS sampler: 0x2d8 */
SINGLE_BIND(161, 0x17) /* GS CB: 0x569 */
SINGLE_BIND(162, 0x1f) /* GS SRV: 0x2c3 */
SINGLE_BIND(163, 0x20) /* GS sampler: 0x67d */
SINGLE_BIND(164, 0x47) /* CS CB: 0 */
SINGLE_BIND(165, 0x43) /* CS SRV: 0xa */
SINGLE_BIND(166, 0x46) /* CS sampler: 0x114 */
SINGLE_BIND(167, 0x3e) /* HS CB: 0x2fb */
SINGLE_BIND(168, 0x3b) /* HS SRV: 0x187 */
SINGLE_BIND(169, 0x3d) /* HS sampler: 0x40f */
SINGLE_BIND(170, 0x42) /* DS CB: 0x432 */
SINGLE_BIND(171, 0x3f) /* DS SRV: 0x223 */
SINGLE_BIND(172, 0x41) /* DS sampler: 0x546 */
#undef SINGLE_BIND

static void automatic_completion(PerformanceContext *context, BOOL enabled) {
    FM5AutoCompletionPolicy policy;
    HRESULT status;
    AcquireSRWLockShared(&cache_lock);
    policy = completion_policy;
    ReleaseSRWLockShared(&cache_lock);
    if (!policy) unsupported(context, 0xbf);
    status = policy(context->native, enabled != FALSE);
    if (FAILED(status)) unsupported(context, 0xbf);
    InterlockedExchange((volatile LONG *)&context->automatic_completion, enabled != FALSE);
}

void FM5ConfigurePerformanceContext(FM5AutoCompletionPolicy policy) {
    AcquireSRWLockExclusive(&cache_lock);
    completion_policy = policy;
    ReleaseSRWLockExclusive(&cache_lock);
}

void FM5ConfigureContextInterop(FM5TopologyPolicy topology, FM5DeviceWrapper device_wrapper) {
    AcquireSRWLockExclusive(&cache_lock);
    topology_policy = topology;
    device_wrapper_policy = device_wrapper;
    ReleaseSRWLockExclusive(&cache_lock);
}

void FM5ConfigureContextResolve(FM5Texture2DDescriptionPolicy description) {
    AcquireSRWLockExclusive(&cache_lock);
    texture_description_policy = description;
    ReleaseSRWLockExclusive(&cache_lock);
}

void FM5ConfigureContextScissors(FM5ScissorSnapshotPolicy snapshot) {
    AcquireSRWLockExclusive(&cache_lock);
    scissor_snapshot_policy = snapshot;
    ReleaseSRWLockExclusive(&cache_lock);
}

void FM5ConfigureContextSamplers(FM5PixelSamplersPolicy getter) {
    AcquireSRWLockExclusive(&cache_lock);
    pixel_samplers_policy=getter;
    ReleaseSRWLockExclusive(&cache_lock);
}

void *FM5UnwrapPerformanceContext(void *value) {
    PerformanceContext *context;
    void *native = value;
    AcquireSRWLockShared(&cache_lock);
    for (context = contexts; context; context = context->next) {
        if (context == value) { native = context->native; break; }
    }
    ReleaseSRWLockShared(&cache_lock);
    return native;
}

HRESULT FM5WrapPerformanceContext(void *native_owned, void **result) {
    PerformanceContext *context;
    if (!result) return E_POINTER;
    *result = NULL;
    if (!native_owned) return E_INVALIDARG;
    AcquireSRWLockExclusive(&cache_lock);
    for (context = contexts; context; context = context->next) {
        if (context == native_owned) {
            *result = context;
            ReleaseSRWLockExclusive(&cache_lock);
            return S_OK;
        }
        if (context->native == native_owned) {
            add_ref(context);
            *result = context;
            ReleaseSRWLockExclusive(&cache_lock);
            release_native(native_owned);
            return S_OK;
        }
    }
    context = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*context));
    if (!context) { ReleaseSRWLockExclusive(&cache_lock); return E_OUTOFMEMORY; }
    context->vtable = context->inline_methods;
    context->references = 1;
    context->native = native_owned;
    context->inline_methods[0] = (void *)query;
    context->inline_methods[1] = (void *)add_ref;
    context->inline_methods[2] = (void *)release;
#define FM5_PERF_SLOT(index) context->inline_methods[index] = (void *)unsupported_##index;
#include "fm5_perf_slots.inc"
#undef FM5_PERF_SLOT
#include "fm5_context_standard_assign.inc"
    context->inline_methods[3] = (void *)get_device;
    context->inline_methods[0x0d] = (void *)draw_indexed;
    context->inline_methods[0x0e] = (void *)draw;
    context->inline_methods[0x0f] = (void *)observed_map;
    context->inline_methods[0x10] = (void *)observed_unmap;
    context->inline_methods[0x14] = (void *)set_index_buffer;
    context->inline_methods[0x15] = (void *)draw_indexed_instanced;
    context->inline_methods[0x16] = (void *)draw_instanced;
    context->inline_methods[0x19] = (void *)set_topology;
    context->inline_methods[0x3a] = (void *)resolve_subresource;
    context->inline_methods[0x61] = (void *)get_scissor_rectangles;
    context->inline_methods[0x4c] = (void *)get_pixel_samplers;
    context->inline_methods[0xad] = (void *)single_vertex_buffer;
    context->inline_methods[0x92] = (void *)wait_resource;
    context->inline_methods[0x93] = (void *)flush_caches;
    context->inline_methods[0x94] = (void *)wait_idle;
    context->inline_methods[0x95] = (void *)insert_fence;
    context->inline_methods[0x96] = (void *)wait_fence;
    context->inline_methods[0x97] = (void *)inherit_cb;
    context->inline_methods[0x98] = (void *)inherit_srv;
#define SINGLE_BIND(old_index, az_index) context->inline_methods[old_index] = (void *)single_##old_index;
    SINGLE_BIND(155, 0x11) SINGLE_BIND(156, 0x09) SINGLE_BIND(157, 0x0b)
    SINGLE_BIND(158, 0x08) SINGLE_BIND(159, 0x19) SINGLE_BIND(160, 0x1a)
    SINGLE_BIND(161, 0x17) SINGLE_BIND(162, 0x1f) SINGLE_BIND(163, 0x20)
    SINGLE_BIND(164, 0x47) SINGLE_BIND(165, 0x43) SINGLE_BIND(166, 0x46)
    SINGLE_BIND(167, 0x3e) SINGLE_BIND(168, 0x3b) SINGLE_BIND(169, 0x3d)
    SINGLE_BIND(170, 0x42) SINGLE_BIND(171, 0x3f) SINGLE_BIND(172, 0x41)
#undef SINGLE_BIND
    context->inline_methods[0xbf] = (void *)automatic_completion;
    context->next = contexts;
    contexts = context;
    *result = context;
    ReleaseSRWLockExclusive(&cache_lock);
    return S_OK;
}
