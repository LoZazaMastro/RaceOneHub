#include "../graphics-port/fm5_fmask_bridge.h"
#include "fm5_msaa4.h"
#include <stdlib.h>
#include "../audio-tls/resource-port/fm5_resource.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
typedef struct Resource {void **table;ULONG refs;UINT desc[13];} Resource;
typedef struct View {void **table;ULONG refs;Resource *resource;} View;
static void *resource_table[13],*view_table[9],*device_table[70];
static void **device=device_table;
static Resource color,auxiliary;
static View view;
static UINT source_samples,source_quality,mask_format,mask_bytes,mask_value;
static ULONG WINAPI resource_release(Resource *r) {
    assert(r->refs);if(!--r->refs) FM5ForgetLegacyResource(r);return r->refs;
}
static void WINAPI native_desc(Resource*r,void*out) {memcpy(out,r->desc,52);((UINT*)out)[6]=0;}
static ULONG WINAPI view_release(View*v) {assert(v->refs);if(!--v->refs)resource_release(v->resource);return v->refs;}
static HRESULT WINAPI create_texture(void*self,const UINT*desc,const void*initial,void**out) {
    Resource*r=desc[5]==1?&auxiliary:&color;
    assert(self==&device && !r->refs);r->table=resource_table;r->refs=1;memcpy(r->desc,desc,52);
    if(r==&auxiliary) {
        const struct {const BYTE*p;UINT row,slice;}*data=initial;
        assert(desc[4]==mask_format && desc[5]==1 && data && data->row==3*mask_bytes && data->slice==6*mask_bytes);
        for(UINT i=0;i<6;++i)assert(mask_bytes==1 ? data->p[i]==mask_value : ((const UINT*)data->p)[i]==mask_value);
    } else {assert(!initial && desc[4]==26 && desc[5]==source_samples && desc[6]==source_quality);r->desc[5]=FM5ForcedSampleCount();r->desc[6]=0xffffffff;}
    *out=r;return S_OK;
}
static HRESULT WINAPI create_view(void*self,void*resource,const void*desc,void**out) {
    assert(self==&device && resource==&auxiliary && ((const UINT*)desc)[0]==mask_format);
    assert(!view.refs);view.table=view_table;view.refs=1;view.resource=resource;++view.resource->refs;
    *out=&view;return S_OK;
}
static void run_case(UINT samples,UINT quality,UINT format) {
    source_samples=samples;source_quality=quality;mask_format=format;mask_bytes=format==62?1:4;mask_value=format==62?0xe4:FM5ForcedSampleCount()==8?0x76543210:0x44443210;
    UINT desc[12]={3,2,1,1,26,2,1,0,0x28,0,0,0},srv[6]={62,4,0,1,0,0},observed[12]={0};
    desc[5]=samples;desc[6]=quality;srv[0]=format;
    void*source,*result;
    resource_table[2]=(void*)resource_release;resource_table[12]=(void*)native_desc;
    view_table[2]=(void*)view_release;device_table[5]=(void*)create_texture;device_table[7]=(void*)create_view;
    assert(FM5CreateLegacyResource(&device,5,desc,NULL,&source)==S_OK);
    assert(source==&color && FM5ResourceMetadataCount()==1);
    assert(FM5CreateLegacyShaderResourceView(&device,source,srv,&result)==S_OK && result==&view);
    assert(FM5ResourceMetadataCount()==2 && color.refs==1 && auxiliary.refs==1);
    /* A real native GetResource returns its owned auxiliary, with AddRef. */
    Resource*exposed=view.resource;++exposed->refs;
    assert(FM5GetLegacyTexture2DDescription(exposed,observed)==S_OK);
    assert(observed[0]==3 && observed[1]==2 && observed[4]==format && observed[5]==1 && observed[6]==0);
    resource_release(exposed);
    assert(FM5GetLegacyTexture2DDescription(source,observed)==S_OK && observed[5]==samples && observed[6]==quality);
    view_release(result);assert(!auxiliary.refs && FM5ResourceMetadataCount()==1);
    resource_release(source);assert(FM5ResourceMetadataCount()==0);

}

int main(void) {
    BYTE *host=(BYTE*)calloc(1,0x319000);assert(host);
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)host;dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    IMAGE_NT_HEADERS64 *pe=(IMAGE_NT_HEADERS64*)(host+0x80);pe->Signature=IMAGE_NT_SIGNATURE;
    pe->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;pe->OptionalHeader.SizeOfImage=0x319000;
    const UINT sites[]={0xe752a,0x130a4e,0x132310,0x1a7e60,0x1ab026};
    for(UINT i=0;i<sizeof(sites)/sizeof(sites[0]);++i)memset(host+sites[i],0x90,2);
    memset(host+0xe7551,0x90,6);*(UINT*)(host+0x318ea0)=4;
    FM5SetMsaaTestHost(host);assert(FM5ForcedFourSampleMode());
    for(UINT forced=4;forced<=8;forced*=2) {
        *(UINT*)(host+0x318ea0)=forced;assert(FM5ForcedSampleCount()==forced);
        for(UINT n=2;n<=4;n*=2)for(UINT q=1;q<=2;++q)run_case(n,q,62);
        run_case(4,3,42);
        for(UINT q=1;q<=3;++q)run_case(8,q,42);
    }
    FM5SetMsaaTestHost(NULL);free(host);
    puts("PASS: 16 forced4/8 FMASK creation/real metadata adapter combinations, exact R8/R32 pixels and row/slice pitches, logical originalCount/Quality preserved, auxiliary GetResource/GetDesc metadata, balanced ownership/no residual records");
    return 0;
}
