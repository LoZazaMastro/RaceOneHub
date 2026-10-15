#include "fm5_apu.h"
#include <stdint.h>
#include <string.h>
#include "apu_guards.h"
typedef HRESULT (WINAPI *CreateFn)(UINT32,UINT32);
typedef HRESULT (WINAPI *AllocFn)(void **,UINT32 *,UINT32,UINT32,UINT32);
typedef HRESULT (WINAPI *ShapeFn)(void *);
typedef HRESULT (WINAPI *HalFn)(void **);
typedef struct Heap {void *cpu;UINT32 bytes;UINT32 apu;void *tree;SIZE_T entries;} Heap;
typedef struct Backends {CreateFn create;AllocFn alloc;ShapeFn shape;HalFn hal_create;BYTE *ready;Heap *heaps;} Backends;
static SRWLOCK bootstrap_lock=SRWLOCK_INIT;
#ifdef FM5_APU_TEST
static Backends test_backend;
void FM5APUSetTestBackends(const FM5APUTestBackends *b){
    C_ASSERT(sizeof(Backends)==sizeof(FM5APUTestBackends));
    AcquireSRWLockExclusive(&bootstrap_lock);
    if(b)memcpy(&test_backend,b,sizeof(test_backend));else memset(&test_backend,0,sizeof(test_backend));
    ReleaseSRWLockExclusive(&bootstrap_lock);
}
#endif
static HRESULT backends(Backends *out){
#ifdef FM5_APU_TEST
    *out=test_backend;
#else
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!base)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)base;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0x40 || dos->e_lfanew>0x1000)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64 *)(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt->OptionalHeader.SizeOfImage<0x37c9e0)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    for(UINT i=0;i<4;++i)if(memcmp(base+apu_guards[i].rva,apu_guards[i].bytes,16))return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    for(UINT i=0;i<7;++i){DWORD value;memcpy(&value,base+apu_parameters[i].rva,4);if(value!=apu_parameters[i].expected)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);}
    out->create=(CreateFn)(uintptr_t)(base+0xaead0);out->alloc=(AllocFn)(uintptr_t)(base+0xaef00);out->shape=(ShapeFn)(uintptr_t)(base+0xb0be0);
    out->hal_create=(HalFn)(uintptr_t)(base+0xb19e0);
    out->ready=base+0x333788;out->heaps=(Heap *)(base+0x37c9a0);
#endif
    if(!out->create || !out->alloc || !out->shape || !out->hal_create || !out->ready || !out->heaps)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    return S_OK;
}
static BOOL initialized(const Backends *b){
    C_ASSERT(sizeof(Heap)==32);
    return *b->ready && b->heaps[0].cpu && b->heaps[1].cpu && b->heaps[0].cpu!=b->heaps[1].cpu &&
        b->heaps[0].bytes==FM5_APU_HEAP1_BYTES && b->heaps[1].bytes==FM5_APU_HEAP2_BYTES &&
        b->heaps[0].apu==FM5_APU_HEAP1_ADDRESS && b->heaps[1].apu==FM5_APU_HEAP2_ADDRESS && b->heaps[0].tree && b->heaps[1].tree;
}
HRESULT WINAPI FM5EnsureApuDefaultHeap(void){
    Backends b;HRESULT hr=backends(&b);if(FAILED(hr))return hr;
    AcquireSRWLockExclusive(&bootstrap_lock);
    if(*b.ready)hr=initialized(&b)?S_FALSE:HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    else {hr=b.create(FM5_APU_HEAP1_BYTES,FM5_APU_HEAP2_BYTES);if(SUCCEEDED(hr)&&!initialized(&b))hr=E_UNEXPECTED;}
    ReleaseSRWLockExclusive(&bootstrap_lock);return hr;
}
HRESULT WINAPI FM5ApuAlloc(void **cpu,UINT32 *apu,UINT32 bytes,UINT32 alignment,UINT32 heap_type){
    if(cpu)*cpu=NULL;if(apu)*apu=0;
    if(!cpu || !bytes || alignment<4 || (alignment&(alignment-1)) || heap_type<1 || heap_type>2)return E_INVALIDARG;
    /* Preserve native maximum-alignment diagnostic rather than inventing a new allocator. */
    Backends b;HRESULT hr=backends(&b);if(FAILED(hr))return hr;
    hr=FM5EnsureApuDefaultHeap();if(FAILED(hr))return hr;
    return b.alloc(cpu,apu,bytes,alignment,heap_type);
}
HRESULT WINAPI FM5AcpHalAllocateShapeContexts(void *inout_contexts){
    if(!inout_contexts)return E_INVALIDARG;
    Backends b;HRESULT hr=backends(&b);if(FAILED(hr))return hr;
    hr=FM5EnsureApuDefaultHeap();if(FAILED(hr))return hr;
    return b.shape(inout_contexts);
}

HRESULT WINAPI FM5AcpHalCreate(void **out){
    if(!out)return E_INVALIDARG;
    *out=NULL;
    Backends b;HRESULT hr=backends(&b);if(FAILED(hr))return hr;
    hr=FM5EnsureApuDefaultHeap();if(FAILED(hr))return hr;
    return b.hal_create(out);
}
