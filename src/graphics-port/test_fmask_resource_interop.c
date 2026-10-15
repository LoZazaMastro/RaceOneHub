#include "fm5_fmask_bridge.h"
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
        assert(desc[4]==62 && desc[5]==1 && data && data->row==3 && data->slice==6);
        for(UINT i=0;i<6;++i)assert(data->p[i]==2);
    } else assert(!initial && desc[4]==26 && desc[5]==2 && desc[6]==1);
    *out=r;return S_OK;
}
static HRESULT WINAPI create_view(void*self,void*resource,const void*desc,void**out) {
    assert(self==&device && resource==&auxiliary && ((const UINT*)desc)[0]==62);
    assert(!view.refs);view.table=view_table;view.refs=1;view.resource=resource;++view.resource->refs;
    *out=&view;return S_OK;
}
int main(void) {
    UINT desc[12]={3,2,1,1,26,2,1,0,0x28,0,0,0},srv[6]={62,4,0,1,0,0},observed[12]={0};
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
    assert(observed[0]==3 && observed[1]==2 && observed[4]==62 && observed[5]==1 && observed[6]==0);
    resource_release(exposed);
    assert(FM5GetLegacyTexture2DDescription(source,observed)==S_OK && observed[5]==2 && observed[6]==1);
    view_release(result);assert(!auxiliary.refs && FM5ResourceMetadataCount()==1);
    resource_release(source);assert(FM5ResourceMetadataCount()==0);
    puts("PASS: actual resource metadata adapter + FMASK auxiliary creation, normalized quality restoration, GetResource/GetDesc auxiliary metadata, balanced ownership and zero residual records");
    return 0;
}
