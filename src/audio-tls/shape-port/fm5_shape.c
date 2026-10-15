#include "fm5_shape.h"
#include <stdio.h>
#include <string.h>

typedef BOOLEAN (WINAPI *Executor)(void *,const void *);
typedef struct GraphJob {BYTE opaque[0x48];const UINT64 *begin;const UINT64 *end;} GraphJob;
C_ASSERT(sizeof(GraphJob)==0x58);
static volatile LONG reported;
static UINT mapped_opcode(UINT64 word) {
    /* Old writers clear four opcode bits. No nonzero old status/flag bit is proven. */
    if(word&0xf0)return 0;
    switch(word&15){case 0:return 1;case 1:return 2;case 4:return 16;default:return 0;}
}
/* Only producer-proven old SDK operations with clear status bits are translated. */
HRESULT FM5TranslateShapeGraph(UINT64 *destination,const UINT64 *source,SIZE_T bytes) {
    if(bytes%sizeof(UINT64) || (bytes && (!source || !destination)))return E_INVALIDARG;
    SIZE_T count=bytes/sizeof(UINT64);
    for(SIZE_T i=0;i<count;++i)if(!mapped_opcode(source[i]))return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    for(SIZE_T i=0;i<count;++i)destination[i]=(source[i]&~(UINT64)31)|mapped_opcode(source[i]);
    return S_OK;
}
#ifdef FM5_SHAPE_TEST
static Executor test_executor;
static FM5ShapeAllocator test_allocate;
static FM5ShapeDeallocator test_free;
void FM5ShapeSetTestBackends(FM5ShapeExecutor executor,FM5ShapeAllocator allocate,FM5ShapeDeallocator deallocate) {
    test_executor=executor;test_allocate=allocate;test_free=deallocate;
}
#endif
static HRESULT backend(Executor *out) {
#ifdef FM5_SHAPE_TEST
    *out=test_executor;
#else
    static const BYTE expected[16]={0x48,0x8b,0xc4,0x48,0x89,0x58,0x18,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56};
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!base)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER *)base;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0x40 || dos->e_lfanew>0x1000)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    IMAGE_NT_HEADERS64 *nt=(IMAGE_NT_HEADERS64 *)(base+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE || nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64 || nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || nt->OptionalHeader.SizeOfImage<0x1906ff || memcmp(base+0x18fbd0,expected,sizeof(expected)))return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    *out=(Executor)(uintptr_t)(base+0x18fbd0);
#endif
    return *out?S_OK:HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
}
static void *allocate_graph(SIZE_T bytes) {
#ifdef FM5_SHAPE_TEST
    return test_allocate?test_allocate(bytes):NULL;
#else
    return HeapAlloc(GetProcessHeap(),0,bytes);
#endif
}
static void free_graph(void *p) {
#ifdef FM5_SHAPE_TEST
    if(test_free)test_free(p);
#else
    HeapFree(GetProcessHeap(),0,p);
#endif
}
static void fail(HRESULT hr) {
    ULONG_PTR parameters[1]={(ULONG_PTR)(UINT32)hr};
    fprintf(stderr,"FM5_SHAPE_GRAPH HRESULT=0x%08lx\n",(unsigned long)hr);fflush(stderr);
    RaiseException(0xe05f0058,EXCEPTION_NONCONTINUABLE,1,parameters);
}
__declspec(dllexport) BOOLEAN WINAPI FM5ExecuteShapeGraph(void *hal,const void *legacy_job) {
    Executor execute=NULL;HRESULT hr=backend(&execute);
    if(FAILED(hr)){fail(hr);return FALSE;}
    if(!hal || !legacy_job){fail(E_INVALIDARG);return FALSE;}
    GraphJob translated;memcpy(&translated,legacy_job,sizeof(translated));
    uintptr_t begin=(uintptr_t)translated.begin,end=(uintptr_t)translated.end;
    if(end<begin || (end-begin)%sizeof(UINT64) || (end!=begin && !begin)){fail(E_INVALIDARG);return FALSE;}
    SIZE_T bytes=(SIZE_T)(end-begin);
    /* Check the entire source before allocation or any native audio side effect. */
    UINT opcode_mask=0;
    for(SIZE_T i=0;i<bytes/sizeof(UINT64);++i){UINT64 word=translated.begin[i];if(!mapped_opcode(word)){fail(HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));return FALSE;}opcode_mask|=(UINT)1<<(word&15);}
    if(!bytes)return execute(hal,&translated);
    UINT64 *copy=(UINT64 *)allocate_graph(bytes);
    if(!copy){fail(E_OUTOFMEMORY);return FALSE;}
    BOOLEAN result=FALSE;
    __try {
        hr=FM5TranslateShapeGraph(copy,translated.begin,bytes);
        if(FAILED(hr))fail(hr);
        translated.begin=copy;translated.end=copy+bytes/sizeof(UINT64);
        if(InterlockedCompareExchange(&reported,1,0)==0){fprintf(stderr,"FM5_SHAPE_GRAPH_TRANSLATED count=%llu old_opcode_mask=0x%x\n",(unsigned long long)(bytes/sizeof(UINT64)),opcode_mask);fflush(stderr);}
        result=execute(hal,&translated);
    } __finally {free_graph(copy);}
    return result;
}
