#include "fm5_performance_context.h"
#include <assert.h>
#include <stdio.h>
typedef struct Native {void **table;ULONG refs;} Native;
static Native native;
static UINT calls;
static HRESULT result;
static HRESULT WINAPI get(void *object,UINT start,UINT count,void **out) {
    assert(object==&native&&start==0xfedcba98u&&count==2);++calls;
    if(FAILED(result))return result;
    out[0]=(void*)(UINT_PTR)0x1234567887654321ull;out[1]=NULL;return S_OK;
}
static ULONG release(void *object){assert(object==&native&&native.refs);return --native.refs;}
typedef void (*Getter)(void *,UINT,UINT,void **);
static void reject(Getter getter,void *proxy,void **out) {
    BOOL caught=FALSE;
    __try {getter(proxy,0xfedcba98u,2,out);}
    __except(GetExceptionCode()==0xe0f50006?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){caught=TRUE;}
    assert(caught);
}
int main(void) {
    void *table[270]={0},*proxy,*out[4]={(void*)7,(void*)1,(void*)2,(void*)8};
    native.table=table;native.refs=1;table[2]=(void*)release;
    assert(FM5WrapPerformanceContext(&native,&proxy)==S_OK);
    Getter getter=(Getter)(*(void***)proxy)[76];
    getter(proxy,0xffffffff,0xffffffff,NULL);assert(!calls);
    FM5ConfigureContextSamplers(get);getter(proxy,0xfedcba98u,2,out+1);
    assert(calls==1&&out[0]==(void*)7&&out[1]==(void*)(UINT_PTR)0x1234567887654321ull&&!out[2]&&out[3]==(void*)8);
    result=E_INVALIDARG;reject(getter,proxy,out+1);assert(calls==2);
    FM5ConfigureContextSamplers(NULL);reject(getter,proxy,out+1);assert(calls==2);
    getter(proxy,0xffffffff,0xffffffff,NULL);assert(calls==2);
    assert(((ULONG(*)(void*))(*(void***)proxy)[2])(proxy)==0&&!native.refs);
    puts("PASS: PSGetSamplers oldslot76, native receiver/start/count/output ABI, NULL output authentic no-op, explicit policy failure/missing policy rejection, reference cleanup");return 0;
}
