#include "fm5_private_data.h"
#include <stdint.h>
#include <string.h>

typedef ULONG (WINAPI *RefFn)(void *);
typedef struct UnknownTable {void *query;RefFn add;RefFn release;} UnknownTable;
typedef struct Property {
    GUID key;UINT bytes;UINT kind;LONG references;void *value;struct Property *next;
} Property;
typedef struct Bag {void *object;Property *properties;struct Bag *next;} Bag;
static SRWLOCK lock=SRWLOCK_INIT;
static Bag *bags[256];
#ifdef FM5_PRIVATE_DATA_TEST
static volatile LONG fail_allocation=-1;
void FM5PrivateDataFailAllocation(LONG countdown) {InterlockedExchange(&fail_allocation,countdown);}
void FM5PrivateDataCounts(SIZE_T *objects,SIZE_T *properties) {
    SIZE_T o=0,p=0;AcquireSRWLockShared(&lock);
    for(UINT i=0;i<256;++i) for(Bag *b=bags[i];b;b=b->next) {++o;for(Property *v=b->properties;v;v=v->next) ++p;}
    ReleaseSRWLockShared(&lock);if(objects)*objects=o;if(properties)*properties=p;
}
#endif
static UINT bucket(void *value) {uintptr_t v=(uintptr_t)value;return (UINT)((v>>4)^(v>>12))&255;}
static void *allocate(SIZE_T bytes) {
#ifdef FM5_PRIVATE_DATA_TEST
    LONG n=InterlockedCompareExchange(&fail_allocation,-1,-1);
    if(n>=0) {if(!n){InterlockedExchange(&fail_allocation,-1);return NULL;}InterlockedDecrement(&fail_allocation);}
#endif
    return HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,bytes);
}
static UnknownTable *table(void *value) {return *(UnknownTable **)value;}
static void release_property(Property *property) {
    if(property && !InterlockedDecrement(&property->references)) {
        if(property->kind) table(property->value)->release(property->value);
        HeapFree(GetProcessHeap(),0,property);
    }
}
static HRESULT set(void *object,REFGUID key,UINT bytes,const void *data,UINT kind) {
    if(!object || !key) return E_INVALIDARG;
    Property *entry=NULL;
    if(data) {
        entry=(Property *)allocate(sizeof(*entry)+(kind?0:bytes));
        if(!entry) return E_OUTOFMEMORY;
        entry->key=*key;entry->bytes=kind?(UINT)sizeof(void *):bytes;entry->kind=kind;entry->references=1;
        if(kind) {entry->value=(void *)data;table(entry->value)->add(entry->value);}
        else {entry->value=entry+1;if(bytes)memcpy(entry->value,data,bytes);}
    }
    UINT hash=bucket(object);
    AcquireSRWLockExclusive(&lock);
    Bag *bag=bags[hash];while(bag && bag->object!=object)bag=bag->next;
    if(!bag && entry) {
        bag=(Bag *)allocate(sizeof(*bag));
        if(!bag) {ReleaseSRWLockExclusive(&lock);release_property(entry);return E_OUTOFMEMORY;}
        bag->object=object;bag->next=bags[hash];bags[hash]=bag;
    }
    Property *old=NULL;
    if(bag) {
        Property **link=&bag->properties;while(*link && memcmp(&(*link)->key,key,sizeof(GUID)))link=&(*link)->next;
        old=*link;
        if(entry) {entry->next=old?old->next:NULL;*link=entry;}
        else if(old) *link=old->next;
    }
    ReleaseSRWLockExclusive(&lock);
    if(old) release_property(old);
    return entry||old?S_OK:FM5_PRIVATE_NOT_FOUND;
}
HRESULT WINAPI FM5SetPrivateData(void *object,REFGUID key,UINT bytes,const void *data) {return set(object,key,bytes,data,0);}
HRESULT WINAPI FM5SetPrivateDataInterface(void *object,REFGUID key,void *value) {return set(object,key,0,value,1);}
HRESULT WINAPI FM5SetPrivateDataInterface2(void *object,REFGUID key,void *value) {return set(object,key,0,value,2);}
HRESULT WINAPI FM5GetPrivateData(void *object,REFGUID key,UINT *bytes,void *data) {
    if(!object || !key || !bytes) return E_INVALIDARG;
    Property *entry=NULL;
    AcquireSRWLockShared(&lock);
    Bag *bag=bags[bucket(object)];while(bag && bag->object!=object)bag=bag->next;
    if(bag) for(entry=bag->properties;entry;entry=entry->next) if(!memcmp(&entry->key,key,sizeof(GUID))) {InterlockedIncrement(&entry->references);break;}
    ReleaseSRWLockShared(&lock);
    if(!entry) {*bytes=0;return FM5_PRIVATE_NOT_FOUND;}
    UINT capacity=*bytes;*bytes=entry->bytes;
    if(entry->kind) {
        table(entry->value)->add(entry->value);
        if(data && capacity>=sizeof(void *)) memcpy(data,&entry->value,sizeof(void *));
    } else if(data && capacity) memcpy(data,entry->value,capacity<entry->bytes?capacity:entry->bytes);
    release_property(entry);return S_OK;
}
void WINAPI FM5ForgetPrivateData(void *object) {
    if(!object)return;
    AcquireSRWLockExclusive(&lock);
    Bag **link=&bags[bucket(object)];while(*link && (*link)->object!=object)link=&(*link)->next;
    Bag *bag=*link;if(bag)*link=bag->next;
    ReleaseSRWLockExclusive(&lock);
    if(bag) {
        Property *property=bag->properties;
        while(property) {Property *next=property->next;release_property(property);property=next;}
        HeapFree(GetProcessHeap(),0,bag);
    }
}
