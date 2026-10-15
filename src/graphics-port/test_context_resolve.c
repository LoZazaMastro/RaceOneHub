#include "fm5_performance_context.h"
#include <assert.h>
#include <stdio.h>

typedef struct Native { void **table; ULONG refs; } Native;
static Native native;
static void *source = (void *)(UINT_PTR)0x1234567887654321ull;
static void *destination = (void *)(UINT_PTR)0x8765432112345678ull;
static UINT samples, copies, resolves, queries;
static HRESULT source_status, destination_status;
static HRESULT WINAPI description(void *resource, void *out) {
    ++queries;
    assert(resource == source || resource == destination);
    ZeroMemory(out,48);
    ((UINT *)out)[5] = resource == source ? samples : 1;
    return resource == source ? source_status : destination_status;
}
static ULONG release(void *object) { assert(object==&native && native.refs); return --native.refs; }
static void copy(void *object,void *dst,UINT di,UINT x,UINT y,UINT z,void *src,UINT si,const void *box) {
    assert(object==&native && dst==destination && src==source);
    assert(di==0xfedcba98u && si==0x89abcdefu && !x && !y && !z && !box);
    ++copies;
}
static void resolve(void *object,void *dst,UINT di,void *src,UINT si,UINT format) {
    assert(object==&native && dst==destination && src==source);
    assert(di==0xfedcba98u && si==0x89abcdefu && format==0xfeedabcdu);
    ++resolves;
}
typedef void (*Resolve)(void *,void *,UINT,void *,UINT,UINT);
static void reject(Resolve call,void *proxy,void *dst,void *src) {
    BOOL rejected=FALSE; UINT before=copies+resolves;
    __try { call(proxy,dst,0xfedcba98u,src,0x89abcdefu,0xfeedabcdu); }
    __except(GetExceptionCode()==0xe0f50006?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) { rejected=TRUE; }
    assert(rejected && before==copies+resolves);
}
int main(void) {
    void *table[270]={0}, *proxy;
    native.table=table; native.refs=1;
    table[2]=(void*)release; table[0x2e]=(void*)copy; table[0x39]=(void*)resolve;
    assert(FM5WrapPerformanceContext(&native,&proxy)==S_OK);
    Resolve call=(Resolve)(*(void***)proxy)[0x3a];
    FM5ConfigureContextResolve(description);
    samples=1; call(proxy,destination,0xfedcba98u,source,0x89abcdefu,0xfeedabcdu);
    assert(copies==1 && resolves==0 && queries==2);
    for(samples=2;samples<=8;samples*=2) call(proxy,destination,0xfedcba98u,source,0x89abcdefu,0xfeedabcdu);
    assert(copies==1 && resolves==3);
    samples=0; reject(call,proxy,destination,source);
    samples=1; source_status=E_NOINTERFACE; reject(call,proxy,destination,source);
    source_status=HRESULT_FROM_WIN32(ERROR_NOT_FOUND); reject(call,proxy,destination,source);
    source_status=S_OK; destination_status=E_NOINTERFACE; reject(call,proxy,destination,source);
    destination_status=HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT); reject(call,proxy,destination,source);
    destination_status=S_OK; reject(call,proxy,NULL,source); reject(call,proxy,destination,NULL);
    FM5ConfigureContextResolve(NULL); reject(call,proxy,destination,source);
    assert(((ULONG(*)(void*))(*(void***)proxy)[2])(proxy)==0 && !native.refs);
    puts("PASS: original single-sample full-copy semantics, unchanged 2/4/8-sample Resolve, all 9/6 arguments, unknown/wrong-kind/getter failure/NULL/missing policy rejection, reference cleanup");
    return 0;
}
