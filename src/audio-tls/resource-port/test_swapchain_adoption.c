#include "fm5_resource.h"
#include "../../graphics-port/fm5_dxgi_bridge.h"
#include <stdio.h>
#include <string.h>
#include "../../../analysis/audio-tls/resource-port/swapchain-test-guard.h"
static unsigned checks,getter_calls,native_calls,releases;
#define CHECK(x) do{++checks;if(!(x)){printf("failed_line=%u\n",__LINE__);return 1;}}while(0)
typedef struct Fake {void **table;LONG refs;} Fake;
static void *table[13],*device_table[65];
static Fake resource={table,0},device={device_table,1};
static DWORD description[11]={1920,1080,1,1,28,1,0,0,0x28,0,0};
static HRESULT native_status=S_OK;
static BOOL partial,null_result;
static int swapchain,iid_token;
static UINT expected_index=0xfedcba98;
void *WINAPI FM5UnwrapPerformanceDevice(void *p){return p;}
static ULONG WINAPI release(void *p){++releases;return (ULONG)InterlockedDecrement(&((Fake *)p)->refs);}
static void WINAPI getter(void *p,void *out){if(p!=&resource)RaiseFailFastException(NULL,NULL,0);++getter_calls;memcpy(out,description,44);}
static HRESULT WINAPI create(void *p,const void *desc,const void *initial,void **out){(void)p;(void)desc;(void)initial;*out=&resource;++resource.refs;return S_OK;}
static HRESULT WINAPI native_buffer(void *p,UINT index,const void *iid,void **out){
 if(p!=&swapchain || index!=expected_index || iid!=&iid_token)RaiseFailFastException(NULL,NULL,0);
 ++native_calls;if(!out)return E_POINTER;*out=NULL;
 if((SUCCEEDED(native_status)&&!null_result)||partial){++resource.refs;*out=&resource;}
 return native_status;
}
int main(void){
 union {void *p;ULONG(WINAPI *ref)(void*);void(WINAPI *get)(void*,void*);HRESULT(WINAPI *create)(void*,const void*,const void*,void**);} call;
 call.ref=release;table[2]=call.p;call.get=getter;table[12]=call.p;call.create=create;device_table[5]=call.p;
 const SIZE_T bytes=0x50000;BYTE *image=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,bytes);CHECK(image!=NULL);
 memcpy(image+0x4f7c0,swapchain_guard,32);void *vt[270],*before[270];
 for(UINT j=0;j<270;++j)vt[j]=(void *)(uintptr_t)(j+1);vt[10]=image+0x4f7c0;memcpy(before,vt,sizeof(vt));
 for(UINT j=0;j<32;++j){image[0x4f7c0+j]^=1;CHECK(FAILED(FM5PatchFactoryVtable(vt,0xf,image,bytes))&&!memcmp(vt,before,sizeof(vt)));image[0x4f7c0+j]^=1;}
 CHECK(FAILED(FM5PatchFactoryVtable(vt,0xf,image,0x4f7df))&&!memcmp(vt,before,sizeof(vt)));
 CHECK(FM5PatchFactoryVtable(vt,0xf,image,bytes)==S_OK);
 for(UINT j=0;j<270;++j)if(j!=10)CHECK(vt[j]==before[j]);
 FM5DXGITestGetBuffer(native_buffer);FM5GetSwapChainBuffer get=(FM5GetSwapChainBuffer)vt[10];
 void *out=(void *)1;DWORD result[14];memset(result,0xcc,sizeof(result));
 CHECK(FM5AdoptLegacyTexture2D(NULL)==E_POINTER);
 CHECK(get(&swapchain,expected_index,&iid_token,NULL)==E_POINTER);
 CHECK(get(&swapchain,expected_index,&iid_token,&out)==S_OK&&out==&resource&&resource.refs==1&&FM5ResourceMetadataCount()==1);
 CHECK(FM5GetLegacyTexture2DDescription(out,result+1)==S_OK&&!memcmp(result+1,description,44)&&result[12]==0&&result[0]==0xcccccccc&&result[13]==0xcccccccc);
 UINT prior=getter_calls;FM5ResourceFailAllocation(0);
 CHECK(get(&swapchain,expected_index,&iid_token,&out)==S_OK&&resource.refs==2&&getter_calls==prior&&FM5ResourceMetadataCount()==1);FM5ResourceFailAllocation(-1);
 release(out);release(out);CHECK(resource.refs==0);FM5ForgetLegacyResource(out);CHECK(!FM5ResourceMetadataCount());
 FM5ResourceFailAllocation(0);out=(void *)1;UINT old_release=releases;
 CHECK(get(&swapchain,expected_index,&iid_token,&out)==E_OUTOFMEMORY&&!out&&resource.refs==0&&releases==old_release+1&&!FM5ResourceMetadataCount());
 void *saved=table[12];table[12]=NULL;CHECK(FAILED(get(&swapchain,expected_index,&iid_token,&out))&&!out&&resource.refs==0);table[12]=saved;
 description[5]=0;CHECK(get(&swapchain,expected_index,&iid_token,&out)==HRESULT_FROM_WIN32(ERROR_INVALID_DATA)&&!out&&resource.refs==0);description[5]=1;
 description[10]=0x20000;CHECK(get(&swapchain,expected_index,&iid_token,&out)==E_NOTIMPL&&!out&&resource.refs==0);description[10]=0;
 native_status=E_FAIL;CHECK(get(&swapchain,expected_index,&iid_token,&out)==E_FAIL&&!out&&!resource.refs&&!FM5ResourceMetadataCount());
 partial=TRUE;old_release=releases;CHECK(get(&swapchain,expected_index,&iid_token,&out)==E_FAIL&&!out&&resource.refs==0&&releases==old_release+1);partial=FALSE;
 native_status=S_OK;null_result=TRUE;CHECK(get(&swapchain,expected_index,&iid_token,&out)==E_UNEXPECTED&&!out&&!resource.refs);null_result=FALSE;
 native_status=S_FALSE;CHECK(get(&swapchain,expected_index,&iid_token,&out)==S_FALSE&&out==&resource&&resource.refs==1);FM5ForgetLegacyResource(out);release(out);native_status=S_OK;
 DWORD legacy[12];memcpy(legacy,description,44);legacy[5]=8;legacy[6]=3;legacy[11]=0xbeef;
 CHECK(FM5CreateLegacyResource(&device,5,legacy,NULL,&out)==S_OK&&resource.refs==1);
 CHECK(get(&swapchain,expected_index,&iid_token,&out)==S_OK&&resource.refs==2&&FM5ResourceMetadataCount()==1);
 CHECK(FM5GetLegacyTexture2DDescription(out,result)==S_OK&&result[5]==8&&result[6]==3&&result[11]==0xbeef);
 release(out);release(out);FM5ForgetLegacyResource(out);
 description[5]=2;description[6]=1;CHECK(get(&swapchain,expected_index,&iid_token,&out)==S_OK);
 CHECK(FM5GetLegacyTexture2DDescription(out,result)==S_OK&&result[5]==2&&result[6]==1&&result[11]==0);
 FM5ForgetLegacyResource(out);release(out);CHECK(resource.refs==0&&!FM5ResourceMetadataCount());
 HeapFree(GetProcessHeap(),0,image);
 printf("{\"passed\":true,\"checks\":%u,\"native_getbuffer_calls\":%u,\"native_getdesc_calls\":%u,\"releases\":%u,\"metadata_remaining\":0}\n",checks,native_calls,getter_calls,releases);return 0;
}
