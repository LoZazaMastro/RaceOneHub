#include "fm5_allocation.h"
#include <stdio.h>
#include <string.h>

typedef HRESULT (WINAPI *PlacementBufferFn)(void *,const void *,void *,void **);
typedef union NativeCall { void *address; PlacementBufferFn placement; } NativeCall;
typedef struct Device { void **table; } Device;
static DWORD expected[7]={0x90,1,4,0,0,0,0xdeadbeef};
static DWORD placement_memory[64];
static Device device;
static LONG calls;
static HRESULT native_result;
static void *native_object;
static HRESULT WINAPI native_placement(void *self,const void *description,void *placement,void **out) {
    if(self!=&device || memcmp(description,expected,24) || placement!=placement_memory || !out || *out)return E_UNEXPECTED;
    ++calls;*out=native_object;return native_result;
}
#define CHECK(x) do {if(!(x)){fprintf(stderr,"failed line %d: %s\n",__LINE__,#x);return 1;}}while(0)
int main(void) {
    void *table[66]={0};NativeCall call;call.placement=native_placement;
    table[62]=call.address;device.table=table;native_object=placement_memory+40;
    void *out=(void *)1;DWORD before[7];memcpy(before,expected,sizeof(before));
    CHECK(FM5CreateNativePlacementBuffer(&device,expected,placement_memory,&out)==S_OK);
    CHECK(calls==1 && out==native_object && !memcmp(before,expected,sizeof(before)));
    native_result=S_FALSE;out=(void *)1;
    CHECK(FM5CreateNativePlacementBuffer(&device,expected,placement_memory,&out)==S_FALSE && calls==2 && out==native_object);
    native_result=E_OUTOFMEMORY;native_object=NULL;out=(void *)1;
    CHECK(FM5CreateNativePlacementBuffer(&device,expected,placement_memory,&out)==E_OUTOFMEMORY && calls==3 && !out);
    out=(void *)1;CHECK(FM5CreateNativePlacementBuffer(NULL,expected,placement_memory,&out)==E_INVALIDARG && !out && calls==3);
    out=(void *)1;CHECK(FM5CreateNativePlacementBuffer(&device,NULL,placement_memory,&out)==E_INVALIDARG && !out && calls==3);
    CHECK(FM5CreateNativePlacementBuffer(&device,expected,placement_memory,NULL)==E_INVALIDARG && calls==3);
    table[62]=NULL;out=(void *)1;CHECK(FM5CreateNativePlacementBuffer(&device,expected,placement_memory,&out)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && !out && calls==3);
    puts("PASS placement buffer canonical24 ABI, placement pointer, native identity, HRESULT and input immutability; no guest/GPU execution");
    return 0;
}
