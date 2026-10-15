#include "fm5_private_data.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
typedef ULONG (WINAPI *RefFn)(void *);
typedef struct Unknown {void *query;RefFn add;RefFn release;} Unknown;
typedef struct Fake {Unknown *table;LONG references;LONG adds;LONG releases;} Fake;
static GUID key={0x0a51ffd4,0x060b,0x4bef,{0xb5,0x93,0x86,0xf1,0x53,0x6f,0x72,0x18}};
static GUID second={0x12345678,0xabcd,0x1234,{1,2,3,4,5,6,7,8}};
static int object_a,object_b,object_c;
static BOOL reenter;
static LONG reentry_result;
static ULONG WINAPI add(void *self) {
    Fake *p=(Fake *)self;InterlockedIncrement(&p->adds);ULONG n=(ULONG)InterlockedIncrement(&p->references);
    if(reenter) {reenter=FALSE;reentry_result=FM5SetPrivateData(&object_a,&key,0,NULL);}
    return n;
}
static ULONG WINAPI release(void *self) {Fake *p=(Fake *)self;InterlockedIncrement(&p->releases);return (ULONG)InterlockedDecrement(&p->references);}
static Unknown table={NULL,add,release};
static volatile LONG thread_failures;
static DWORD WINAPI worker(void *context) {
    UINT64 value=(UINT64)(uintptr_t)context;UINT64 out=0;
    for(UINT i=0;i<1000;++i) {
        UINT bytes=8;
        if(FM5SetPrivateData(context,&key,8,&value)!=S_OK || FM5GetPrivateData(context,&key,&bytes,&out)!=S_OK || bytes!=8 || out!=value || FM5SetPrivateData(context,&key,0,NULL)!=S_OK) InterlockedIncrement(&thread_failures);
    }
    FM5ForgetPrivateData(context);return 0;
}
static UINT checks;
#define CHECK(x) do {++checks;if(!(x)){printf("failed_line=%u\n",__LINE__);return 1;}}while(0)
int main(void) {
    UINT bytes=8;BYTE buffer[24];memset(buffer,0xcc,sizeof(buffer));
    CHECK(FM5GetPrivateData(NULL,&key,&bytes,buffer)==E_INVALIDARG);
    CHECK(FM5GetPrivateData(&object_a,NULL,&bytes,buffer)==E_INVALIDARG);
    CHECK(FM5GetPrivateData(&object_a,&key,NULL,buffer)==E_INVALIDARG);
    CHECK(FM5SetPrivateData(NULL,&key,0,NULL)==E_INVALIDARG);
    CHECK(FM5SetPrivateData(&object_a,NULL,0,NULL)==E_INVALIDARG);
    CHECK(FM5GetPrivateData(&object_a,&key,&bytes,buffer)==FM5_PRIVATE_NOT_FOUND && bytes==0 && buffer[0]==0xcc);
    CHECK(FM5SetPrivateData(&object_a,&key,8,NULL)==FM5_PRIVATE_NOT_FOUND);
    UINT64 value=0x1122334455667788,copy=value;
    FM5PrivateDataFailAllocation(0);CHECK(FM5SetPrivateData(&object_a,&key,8,&value)==E_OUTOFMEMORY);
    FM5PrivateDataFailAllocation(1);CHECK(FM5SetPrivateData(&object_a,&key,8,&value)==E_OUTOFMEMORY);
    SIZE_T objects=1,properties=1;FM5PrivateDataCounts(&objects,&properties);CHECK(!objects&&!properties);
    CHECK(FM5SetPrivateData(&object_a,&key,8,&value)==S_OK);value=0;
    bytes=8;CHECK(FM5GetPrivateData(&object_a,&key,&bytes,buffer+4)==S_OK && bytes==8 && !memcmp(buffer+4,&copy,8) && buffer[3]==0xcc && buffer[12]==0xcc);
    bytes=0;CHECK(FM5GetPrivateData(&object_a,&key,&bytes,NULL)==S_OK && bytes==8);
    memset(buffer,0xcc,sizeof(buffer));bytes=3;CHECK(FM5GetPrivateData(&object_a,&key,&bytes,buffer+4)==S_OK && bytes==8 && !memcmp(buffer+4,&copy,3) && buffer[7]==0xcc);
    FM5PrivateDataFailAllocation(0);CHECK(FM5SetPrivateData(&object_a,&key,8,&value)==E_OUTOFMEMORY);
    bytes=8;CHECK(FM5GetPrivateData(&object_a,&key,&bytes,buffer)==S_OK && !memcmp(buffer,&copy,8));
    CHECK(FM5SetPrivateData(&object_a,&second,0,&value)==S_OK);
    bytes=8;CHECK(FM5GetPrivateData(&object_a,&second,&bytes,buffer)==S_OK && bytes==0);
    Fake first={&table,1,0,0},other={&table,1,0,0};void *out=NULL;
    CHECK(FM5SetPrivateDataInterface(&object_a,&key,&first)==S_OK && first.references==2);
    bytes=8;CHECK(FM5GetPrivateData(&object_a,&key,&bytes,&out)==S_OK && bytes==8 && out==&first && first.references==3);release(out);
    bytes=0;CHECK(FM5GetPrivateData(&object_a,&key,&bytes,NULL)==S_OK && bytes==8 && first.references==3);release(&first);
    memset(buffer,0xcc,sizeof(buffer));bytes=4;CHECK(FM5GetPrivateData(&object_a,&key,&bytes,buffer)==S_OK && bytes==8 && buffer[0]==0xcc && first.references==3);release(&first);
    CHECK(FM5SetPrivateDataInterface2(&object_a,&key,&other)==S_OK && first.references==1 && other.references==2);
    CHECK(FM5SetPrivateDataInterface2(&object_a,&key,NULL)==S_OK && other.references==1);
    CHECK(FM5SetPrivateDataInterface(&object_a,&key,NULL)==FM5_PRIVATE_NOT_FOUND);
    FM5PrivateDataFailAllocation(1);CHECK(FM5SetPrivateDataInterface(&object_b,&key,&first)==E_OUTOFMEMORY && first.references==1);
    CHECK(FM5SetPrivateDataInterface(&object_a,&key,&first)==S_OK && first.references==2);
    reenter=TRUE;reentry_result=E_FAIL;bytes=8;out=NULL;
    CHECK(FM5GetPrivateData(&object_a,&key,&bytes,&out)==S_OK && out==&first && reentry_result==S_OK && first.references==2);release(out);
    CHECK(first.references==1);
    CHECK(FM5SetPrivateDataInterface(&object_a,&key,&first)==S_OK && first.references==2);
    FM5ForgetPrivateData(&object_a);CHECK(first.references==1);
    FM5ForgetPrivateData(&object_a);FM5ForgetPrivateData(NULL);
    CHECK(FM5SetPrivateData(&object_a,&key,8,&copy)==S_OK);
    bytes=8;CHECK(FM5GetPrivateData(&object_a,&key,&bytes,buffer)==S_OK && !memcmp(buffer,&copy,8));
    CHECK(FM5SetPrivateData(&object_c,&key,8,&value)==S_OK);
    FM5ForgetPrivateData(&object_a);
    bytes=8;CHECK(FM5GetPrivateData(&object_c,&key,&bytes,buffer)==S_OK && !memcmp(buffer,&value,8));
    FM5ForgetPrivateData(&object_c);FM5ForgetPrivateData(&object_b);
    HANDLE threads[4];int thread_objects[4];
    for(UINT i=0;i<4;++i) {threads[i]=CreateThread(NULL,0,worker,&thread_objects[i],0,NULL);CHECK(threads[i]!=NULL);}
    CHECK(WaitForMultipleObjects(4,threads,TRUE,15000)==WAIT_OBJECT_0);
    for(UINT i=0;i<4;++i)CloseHandle(threads[i]);
    FM5PrivateDataCounts(&objects,&properties);CHECK(thread_failures==0 && !objects && !properties && first.references==1 && other.references==1);
    printf("{\"passed\":true,\"checks\":%u,\"concurrent_set_get_remove_cycles\":4000,\"reentrant_interface_get\":true,\"objects_remaining\":0,\"properties_remaining\":0}\n",checks);
    return 0;
}
