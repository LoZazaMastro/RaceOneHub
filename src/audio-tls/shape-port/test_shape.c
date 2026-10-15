#include "fm5_shape.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct Job {BYTE opaque[0x48];const UINT64 *begin,*end;} Job;
static UINT checks,allocations,frees,calls;
static BOOL oom,throw_native;
static BOOLEAN native_result;
static Job *original;
static BYTE original_bytes[0x58];
static UINT64 original_words[5];
static const UINT64 expected[5]={0x200802001,0x400802001,0x8002000002,0x4010,0x8008010};
static int fake_hal;
#define CHECK(x) do {++checks;if(!(x)){printf("failed_line=%u\n",__LINE__);return 1;}}while(0)
static void *allocate(SIZE_T bytes){++allocations;return oom?NULL:malloc(bytes);}
static void deallocate(void *p){++frees;free(p);}
static BOOLEAN WINAPI execute(void *hal,const void *p) {
    const Job *job=(const Job *)p;++calls;
    if(hal!=&fake_hal || p==original || memcmp(job->opaque,original->opaque,0x48) || memcmp(original,original_bytes,sizeof(original_bytes)))RaiseException(0xe0001111,0,0,NULL);
    if(job->begin!=job->end && (job->begin==original->begin || job->end-job->begin!=5 || memcmp(job->begin,expected,sizeof(expected)) || memcmp(original->begin,original_words,sizeof(original_words))))RaiseException(0xe0001112,0,0,NULL);
    if(throw_native)RaiseException(0xe0001113,0,0,NULL);
    return native_result;
}
static DWORD error_code;static HRESULT detail;
static int filter(EXCEPTION_POINTERS *p) {
    error_code=p->ExceptionRecord->ExceptionCode;
    detail=p->ExceptionRecord->NumberParameters?(HRESULT)p->ExceptionRecord->ExceptionInformation[0]:0;
    return EXCEPTION_EXECUTE_HANDLER;
}
static BOOL invoke(Job *job) {
    BOOL caught=FALSE;error_code=0;detail=0;
    __try{(void)FM5ExecuteShapeGraph(&fake_hal,job);}__except(filter(GetExceptionInformation())){caught=TRUE;}
    return caught;
}
int main(void) {
    UINT64 source[5]={0x200802000,0x400802000,0x8002000001,0x4004,0x8008004};
    UINT64 buffer[7];memset(buffer,0xa5,sizeof(buffer));
    CHECK(FM5TranslateShapeGraph(buffer+1,source,sizeof(source))==S_OK);
    CHECK(!memcmp(buffer+1,expected,sizeof(expected)));
    CHECK(buffer[0]==0xa5a5a5a5a5a5a5a5 && buffer[6]==buffer[0]);
    CHECK(FM5TranslateShapeGraph(NULL,NULL,0)==S_OK);
    CHECK(FM5TranslateShapeGraph(NULL,source,8)==E_INVALIDARG);
    CHECK(FM5TranslateShapeGraph(buffer,NULL,8)==E_INVALIDARG);
    CHECK(FM5TranslateShapeGraph(buffer,source,7)==E_INVALIDARG);
    for(UINT i=0;i<32;++i){UINT64 word=0xfedcba9876543200|i,out=0x1122334455667788;BOOL supported=i==0 || i==1 || i==4;HRESULT hr=FM5TranslateShapeGraph(&out,&word,8);CHECK(hr==(supported?S_OK:HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED)));CHECK(out==(supported?((word&~(UINT64)31)|((UINT64)1<<i)):0x1122334455667788));}
    for(UINT bit=4;bit<8;++bit){UINT64 word=(UINT64)1<<bit,out=0x1122334455667788;CHECK(FM5TranslateShapeGraph(&out,&word,8)==HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));CHECK(out==0x1122334455667788);}
    UINT64 invalid[2]={0,31};UINT64 untouched[2]={0xcccc,0xdddd};
    CHECK(FM5TranslateShapeGraph(untouched,invalid,sizeof(invalid))==HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));
    CHECK(untouched[0]==0xcccc && untouched[1]==0xdddd);
    Job job;memset(&job,0xcc,sizeof(job));job.begin=source;job.end=source+5;original=&job;
    memcpy(original_bytes,&job,sizeof(job));memcpy(original_words,source,sizeof(source));
    FM5ShapeSetTestBackends(execute,allocate,deallocate);
    native_result=FALSE;CHECK(FM5ExecuteShapeGraph(&fake_hal,&job)==FALSE);CHECK(calls==1 && allocations==1 && frees==1);
    native_result=TRUE;CHECK(FM5ExecuteShapeGraph(&fake_hal,&job)==TRUE);CHECK(calls==2 && allocations==2 && frees==2);
    CHECK(!memcmp(&job,original_bytes,sizeof(job)) && !memcmp(source,original_words,sizeof(source)));
    throw_native=TRUE;CHECK(invoke(&job));CHECK(error_code==0xe0001113);CHECK(calls==3 && allocations==3 && frees==3);throw_native=FALSE;
    oom=TRUE;CHECK(invoke(&job));CHECK(error_code==0xe05f0058 && detail==E_OUTOFMEMORY);CHECK(calls==3 && allocations==4 && frees==3);oom=FALSE;
    Job bad=job;bad.begin=invalid;bad.end=invalid+2;CHECK(invoke(&bad));CHECK(detail==HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED));CHECK(calls==3 && allocations==4);
    bad=job;bad.end=source-1;CHECK(invoke(&bad));CHECK(detail==E_INVALIDARG);
    bad=job;bad.end=(const UINT64 *)((const BYTE *)source+7);CHECK(invoke(&bad));CHECK(detail==E_INVALIDARG);
    bad=job;bad.begin=NULL;bad.end=(const UINT64 *)8;CHECK(invoke(&bad));CHECK(detail==E_INVALIDARG);
    CHECK(invoke(NULL));CHECK(detail==E_INVALIDARG);
    job.begin=job.end=source;memcpy(original_bytes,&job,sizeof(job));CHECK(FM5ExecuteShapeGraph(&fake_hal,&job)==TRUE);CHECK(calls==4 && allocations==4 && frees==3);
    job.begin=job.end=NULL;memcpy(original_bytes,&job,sizeof(job));CHECK(FM5ExecuteShapeGraph(&fake_hal,&job)==TRUE);CHECK(calls==5 && allocations==4 && frees==3);
    FM5ShapeSetTestBackends(NULL,allocate,deallocate);CHECK(invoke(&job));CHECK(detail==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT));
    printf("{\"checks\":%u,\"calls\":%u,\"allocations\":%u,\"frees\":%u,\"passed\":true}\n",checks,calls,allocations,frees);return 0;
}
