#include "fm5_scissor_state.h"
#include <string.h>
#include <stddef.h>

typedef char LockRecordSize[(sizeof(FM5ScissorLockRecord)==16)?1:-1];
typedef char LockRecordHeldOffset[(offsetof(FM5ScissorLockRecord,held)==8)?1:-1];

typedef void (*WaitForWorker)(void *,UINT64);
typedef FM5ScissorLockRecord *(*AcquireNativeLock)(FM5ScissorLockRecord *,void *);
typedef ULONG (*AddRefSampler)(void *);
typedef struct Guard { SIZE_T rva; BYTE bytes[32]; } Guard;
static const Guard guards[]={
    {0x1b4ef0,{0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x48,0x81,0xec,0x80,0x00,0x00,0x00,0x48,0x8b,0x05,0x3a,0x33,0x16,0x00,0x48,0x33,0xc4,0x48,0x89,0x44,0x24,0x78,0x49,0x8b}},
    {0x1b4e40,{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x4d,0x8b,0xd0,0x8b,0xfa,0x44,0x8b,0xc2,0x4c,0x8d,0x99,0x40,0x71,0x00,0x00,0x49,0xc1,0xe0,0x04,0x48,0x8b,0xd9}},
    {0xef960,{0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,0x48,0x81,0xec,0x00,0x01,0x00,0x00,0x48,0x8b,0x05,0xc7,0x88,0x22,0x00,0x48,0x33,0xc4,0x48,0x89,0x84,0x24}},
    {0x1b61f0,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7c,0x24,0x20,0x55,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0xac,0x24,0xb0,0xce,0xff,0xff}},
    {0xdbe90,{0x48,0x85,0xd2,0x74,0x71,0x48,0x89,0x5c,0x24,0x18,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xfa,0x48,0x8b,0x41,0x10,0x48,0x8b,0x58,0x30,0x48,0x81,0xc3,0x00,0x6e,0x00}},
    {0x589b0,{0x48,0x85,0xd2,0x74,0x37,0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0xf9,0x48,0x8b,0xda,0x48,0x8b,0x49,0x30,0x48,0x81,0xc1,0x40,0x6f,0x00,0x00}},
    {0xc2f00,{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x4c,0x24,0x08,0x57,0x48,0x83,0xec,0x50,0x48,0x8b,0xf9,0x48,0x8d,0x9a,0x98,0xd6,0x00,0x00,0x48,0x89}}
};
static const Guard sampler_guards[]={
    {0xf5cb0,{0x48,0x83,0xec,0x38,0x4c,0x89,0x4c,0x24,0x20,0x45,0x8b,0xc8,0x44,0x8b,0xc2,0xba,0x04,0x00,0x00,0x00,0xe8,0x57,0xce,0x0b,0x00,0x90,0x48,0x83,0xc4,0x38,0xc3,0xcc}},
    {0x1b29a0,{0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x41,0x54,0x41,0x55,0x41,0x56,0x48,0x83,0xec,0x20,0x8b,0xc2,0x48,0x8d,0x99,0x20,0x66,0x00,0x00,0x44,0x8b,0xd2,0x4c,0x8d,0xb1}},
    {0xda960,{0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0xe8,0xe2,0xe8,0xff,0xff,0xff,0xc8,0x83,0xf8,0x10,0x77,0x26,0x48,0x98,0x48,0x8d,0x0d,0x82,0x56,0xf2,0xff,0x0f,0xb6}}
};

static HRESULT copy_cache(BYTE *context,UINT *count,void *rectangles) {
    UINT actual=*(const UINT*)(context+0x7244);
    if(actual>16)return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    CopyMemory(rectangles,context+0x7140,(SIZE_T)actual*sizeof(RECT));
    *count=actual;
    return S_OK;
}

static void release_lock(FM5ScissorLockRecord *guard) {
    if(guard->held) {
        BYTE *mutex=(BYTE*)guard->mutex;
        /* Exact inline destructor used by native getters1b4d40/1b5a40:
         * decrement recursive depth, clear owner and release SRW at zero. */
        if(--*(UINT*)(mutex+12)==0) {
            *(UINT*)(mutex+8)=0;
            ReleaseSRWLockExclusive(guard->mutex);
        }
        guard->held=0;
    }
}

static HRESULT copy_pixel_samplers(BYTE *context,UINT start,UINT count,void **out,AddRefSampler add_ref) {
    void *selected[16];
    BYTE *device=*(BYTE**)(context+0xc40);
    /* f5cb0 selects PSstage4. Backend1b29a0 uses6620+(stage*16+slot)*8.
     * Match the native setter's kind/owner checks before taking any refs. */
    CopyMemory(selected,context+0x6820+(SIZE_T)start*8,(SIZE_T)count*8);
    for(UINT i=0;i<count;++i) if(selected[i]) {
        BYTE *sampler=(BYTE*)selected[i];
        if(*(UINT*)(sampler+0x38)!=0xa || *(BYTE**)(sampler+0x30)!=device)
            return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    for(UINT i=0;i<count;++i)if(selected[i])add_ref(selected[i]);
    CopyMemory(out,selected,(SIZE_T)count*8);
    return S_OK;
}

static HRESULT pixel_samplers(void *native_context,UINT start,UINT count,void **out,
    const BYTE *host,WaitForWorker wait,AcquireNativeLock acquire,AddRefSampler add_ref) {
    BYTE *context=(BYTE*)native_context,*device;
    if(!out)return S_OK;
    if(start>16 || count>16-start)return E_INVALIDARG;
    if(!count)return S_OK;
    if(!context)return E_POINTER;
    if(!host || !wait || !add_ref)return E_UNEXPECTED;
    for(UINT i=0;i<sizeof(guards)/sizeof(guards[0]);++i)
        if(memcmp(host+guards[i].rva,guards[i].bytes,32))return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    for(UINT i=0;i<sizeof(sampler_guards)/sizeof(sampler_guards[0]);++i)
        if(memcmp(host+sampler_guards[i].rva,sampler_guards[i].bytes,32))return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    void **table=*(void***)context;
    if(!table || table[3]!=(void*)(host+0xdbe90) || table[11]!=(void*)(host+0xf5cb0))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    device=*(BYTE**)(context+0xc40);
    if(!device)return E_UNEXPECTED;
    if(context[0xc48]) {
        FM5ScissorLockRecord guard={0};HRESULT status;
        if(context[0xc49])return E_NOTIMPL;
        if(!acquire)return E_UNEXPECTED;
        acquire(&guard,context);
        __try {status=copy_pixel_samplers(context,start,count,out,add_ref);}
        __finally {release_lock(&guard);}
        return status;
    }
    if(context!=device+0x6f40)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    wait(device,*(volatile UINT64*)(device+0x1080));
    MemoryBarrier();
    return copy_pixel_samplers(context,start,count,out,add_ref);
}

static HRESULT snapshot(void *native_context,UINT *count,void *rectangles,
                        const BYTE *host,WaitForWorker wait,AcquireNativeLock acquire) {
    BYTE *context=(BYTE*)native_context;
    BYTE *device;
    UINT64 published;
    if(!context || !count || !rectangles)return E_POINTER;
    if(!host || !wait)return E_UNEXPECTED;
    for(UINT i=0;i<sizeof(guards)/sizeof(guards[0]);++i)
        if(memcmp(host+guards[i].rva,guards[i].bytes,sizeof(guards[i].bytes)))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    void **table=*(void***)context;
    if(!table || table[3]!=(void*)(host+0xdbe90) || table[45]!=(void*)(host+0x1b4ef0))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    /* A non-recording deferred context updates this same cache synchronously
     * under c2f00's recursive native SRW lock. It does not need a GPU/CPU queue
     * drain. Recording contexts retain a stream, not current RECTs. */
    if(context[0xc48]) {
        FM5ScissorLockRecord guard={0};
        HRESULT status;
        if(context[0xc49])return E_NOTIMPL;
        if(!acquire)return E_UNEXPECTED;
        acquire(&guard,context);
        __try { status=copy_cache(context,count,rectangles); }
        __finally { release_lock(&guard); }
        return status;
    }
    device=*(BYTE**)(context+0xc40);
    if(!device)return E_UNEXPECTED;
    /* Native GetImmediateContext589b0 returns the inline object at6f40,
     * the same receiver used by the worker. No extra pointer indirection. */
    if(context!=device+0x6f40)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    /* Constructor5a900 aliases queue head to device+1040. Published sequence
     * is head+40 = device+1080, NOT the potentially unpublished produced1040.
     * ef960 waits completed1180 and performs no D3D12/GPU fence operation. */
    published=*(volatile UINT64*)(device+0x1080);
    wait(device,published);
    MemoryBarrier();
    return copy_cache(context,count,rectangles);
}

HRESULT WINAPI FM5ReadNativeScissorSnapshot(void *native_context,UINT *count,void *rectangles256) {
    const BYTE *host=(const BYTE*)GetModuleHandleW(NULL);
    return snapshot(native_context,count,rectangles256,host,
                    host?(WaitForWorker)(host+0xef960):NULL,
                    host?(AcquireNativeLock)(host+0xc2f00):NULL);
}
HRESULT WINAPI FM5GetNativePixelSamplers(void *native_context,UINT start,UINT count,void **samplers) {
    const BYTE *host=(const BYTE*)GetModuleHandleW(NULL);
    return pixel_samplers(native_context,start,count,samplers,host,
        host?(WaitForWorker)(host+0xef960):NULL,
        host?(AcquireNativeLock)(host+0xc2f00):NULL,
        host?(AddRefSampler)(host+0xda960):NULL);
}
#ifdef FM5_SCISSOR_STATE_TEST
HRESULT FM5TestReadNativeScissorSnapshot(void *native_context,UINT *count,void *rectangles256,
                                      const BYTE *host,FM5ScissorWaitForTest wait) {
    return snapshot(native_context,count,rectangles256,host,wait,NULL);
}
HRESULT FM5TestReadNativeScissorSnapshotLocked(void *native_context,UINT *count,void *rectangles256,
    const BYTE *host,FM5ScissorWaitForTest wait,FM5ScissorLockForTest acquire) {
    return snapshot(native_context,count,rectangles256,host,wait,acquire);
}
HRESULT FM5TestGetNativePixelSamplers(void *native_context,UINT start,UINT count,void **samplers,
    const BYTE *host,FM5ScissorWaitForTest wait,FM5ScissorLockForTest acquire,FM5SamplerAddRefForTest add_ref) {
    return pixel_samplers(native_context,start,count,samplers,host,wait,acquire,add_ref);
}
#endif
