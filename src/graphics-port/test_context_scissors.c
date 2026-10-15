#include "fm5_performance_context.h"
#include <assert.h>
#include <stdio.h>

typedef struct Native { void **table; ULONG refs; } Native;
static Native native;
static RECT state[16];
static UINT bound, snapshots;
static HRESULT snapshot_status;
static HRESULT WINAPI snapshot(void *context,UINT *count,void *out) {
    assert(context==&native); ++snapshots;
    if(FAILED(snapshot_status))return snapshot_status;
    *count=bound; CopyMemory(out,state,sizeof(state)); return S_OK;
}
static ULONG release(void *context) { assert(context==&native && native.refs);return --native.refs; }
typedef void (*Getter)(void *,UINT *,RECT *);
static void reject(Getter get,void *proxy,UINT *count,RECT *rects) {
    BOOL caught=FALSE;
    __try { get(proxy,count,rects); }
    __except(GetExceptionCode()==0xe0f50006?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) { caught=TRUE; }
    assert(caught);
}
int main(void) {
    void *table[270]={0},*proxy;
    native.table=table;native.refs=1;table[2]=(void*)release;
    assert(FM5WrapPerformanceContext(&native,&proxy)==S_OK);
    Getter get=(Getter)(*(void***)proxy)[0x61];
    FM5ConfigureContextScissors(snapshot);
    for(UINT i=0;i<16;++i) {state[i].left=(LONG)(0x80000000u+i);state[i].top=-(LONG)i;state[i].right=0x7fffffff-(LONG)i;state[i].bottom=(LONG)(i*12345);}
    BYTE *pages=VirtualAlloc(NULL,8192,MEM_COMMIT|MEM_RESERVE,PAGE_READWRITE);assert(pages);
    DWORD old;assert(VirtualProtect(pages+4096,4096,PAGE_NOACCESS,&old));
    UINT cases=0;
    for(bound=0;bound<=16;++bound) {
        UINT query=0xfedcba98;get(proxy,&query,NULL);assert(query==bound);
        for(UINT capacity=0;capacity<=17;++capacity) {
            RECT *out=(RECT*)(pages+4096-(SIZE_T)capacity*sizeof(RECT));
            FillMemory(pages,4096,0xcd);UINT count=capacity;
            get(proxy,&count,out);
            UINT copied=capacity<bound?capacity:bound;
            assert(count==copied);
            assert(!memcmp(out,state,(SIZE_T)copied*sizeof(RECT)));
            for(UINT j=copied;j<capacity;++j) {RECT zero={0};assert(!memcmp(out+j,&zero,sizeof(zero)));}
            assert(*((BYTE*)out-1)==0xcd);++cases;
        }
    }
    RECT unchanged={1,2,3,4},out=unchanged;UINT count=1;
    snapshot_status=E_FAIL;reject(get,proxy,&count,&out);assert(count==1&&!memcmp(&out,&unchanged,sizeof(out)));
    snapshot_status=S_OK;bound=17;reject(get,proxy,&count,&out);assert(count==1&&!memcmp(&out,&unchanged,sizeof(out)));
    UINT before=snapshots;reject(get,proxy,NULL,&out);assert(snapshots==before);
    FM5ConfigureContextScissors(NULL);reject(get,proxy,&count,&out);assert(snapshots==before);
    VirtualFree(pages,0,MEM_RELEASE);
    assert(((ULONG(*)(void*))(*(void***)proxy)[2])(proxy)==0&&!native.refs);
    printf("PASS: %u scissor bound/capacity combinations, query-only actual count, original min-count and zero-fill semantics, signed RECT bytes, guard-page boundaries, failed snapshots, lifetime cleanup\n",cases);
    return 0;
}
