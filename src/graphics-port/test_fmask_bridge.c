#include "fm5_fmask_bridge.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static UINT source[12], requested[6], expected_value, expected_bytes;
static int source_object;
static void *device_table[70],*aux_table[13],*view_table[9];
static void **device=device_table,**aux=aux_table,**view=view_table;
static LONG aux_refs,view_refs;
static unsigned textures,views,original_views;
static HRESULT metadata_status=S_OK,texture_status=S_OK,view_status=S_OK;
static BOOL texture_partial,view_partial;

HRESULT WINAPI FM5GetLegacyTexture2DDescription(void *resource,void *out48) {
    assert(resource==&source_object);
    if(SUCCEEDED(metadata_status)) memcpy(out48,source,sizeof(source));
    return metadata_status;
}
static ULONG WINAPI aux_release(void *self) { assert(self==&aux && aux_refs>0);return (ULONG)--aux_refs; }
static ULONG WINAPI view_release(void *self) {
    assert(self==&view && view_refs>0);
    if(!--view_refs) aux_release(&aux);
    return (ULONG)view_refs;
}
static HRESULT WINAPI texture_create(void *self,const void *description,const void *data,void **out) {
    const UINT *desc=description;
    const struct {const void *p;UINT row,slice;} *initial=data;
    SIZE_T bytes=(SIZE_T)source[0]*source[1]*expected_bytes;
    assert(self==&device && desc && initial && out);
    assert(desc[0]==source[0] && desc[1]==source[1] && desc[2]==1 && desc[3]==1);
    assert(desc[4]==requested[0] && desc[5]==1 && desc[6]==0 && desc[7]==0 && desc[8]==8);
    assert(!desc[9]&&!desc[10]&&!desc[11]&&!desc[12]);
    assert(initial->row==source[0]*expected_bytes && initial->slice==bytes);
    for(SIZE_T i=0;i<bytes/expected_bytes;++i)
        assert(expected_bytes==1?((BYTE*)initial->p)[i]==expected_value:((UINT*)initial->p)[i]==expected_value);
    ++textures;*out=NULL;
    if(SUCCEEDED(texture_status)||texture_partial) {assert(!aux_refs);aux_refs=1;*out=&aux;}
    return texture_status;
}
HRESULT WINAPI FM5CreateLegacyResource(void *self,UINT slot,const void *description,const void *data,void **out) {
    UINT canonical[13]={0};
    assert(slot==5);
    memcpy(canonical,description,48);
    return texture_create(self,canonical,data,out);
}
static HRESULT WINAPI view_create(void *self,void *resource,const void *description,void **out) {
    assert(self==&device && description==requested);
    if(resource==&source_object) {++original_views;if(out)*out=NULL;return S_FALSE;}
    assert(resource==&aux && aux_refs==1);++views;
    if(!out) return S_FALSE;
    *out=NULL;
    if(SUCCEEDED(view_status)||view_partial) {assert(!view_refs);view_refs=1;++aux_refs;*out=&view;}
    return view_status;
}
static void reset(UINT samples,UINT quality,UINT format) {
    memset(source,0,sizeof(source));source[0]=3;source[1]=2;source[2]=1;source[3]=1;
    source[4]=26;source[5]=samples;source[6]=quality;source[8]=0x28;
    memset(requested,0,sizeof(requested));requested[0]=format;requested[1]=4;requested[3]=1;
    metadata_status=texture_status=view_status=S_OK;texture_partial=view_partial=FALSE;
}
int main(void) {
    FM5FMaskPlan plan;void *out;unsigned successes=0;
    device_table[5]=(void*)texture_create;device_table[7]=(void*)view_create;
    aux_table[2]=(void*)aux_release;view_table[2]=(void*)view_release;
    assert(FM5SelectExpandedFMask(NULL,requested,&plan)==E_POINTER);
    for(UINT n=2;n<=8;n*=2) for(UINT q=0;q<6;++q) {
        reset(n,q,n==8?42:62);
        BOOL valid=(n==2&&q==1)||(n==4&&q>=1&&q<=2)||(n==8&&q>=1&&q<=3);
        HRESULT hr=FM5SelectExpandedFMask(source,requested,&plan);
        if(valid) {
            assert(hr==S_OK && plan.fragments==n && plan.coverage_samples==n);
            expected_value=n==2?2:n==4?0xe4:0x76543210;expected_bytes=n==8?4:1;
            assert(plan.expanded_value==expected_value && plan.element_bytes==expected_bytes);
            out=(void*)1;
            assert(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,&out)==S_OK && out==&view);
            assert(aux_refs==1 && view_refs==1);view_release(out);assert(!aux_refs&&!view_refs);++successes;
        } else { assert(hr==E_NOTIMPL); }
    }
    assert(successes==6 && textures==6 && views==6);
    reset(2,1,62);expected_value=2;expected_bytes=1;
    assert(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,NULL)==S_FALSE && !aux_refs);
    FM5FMaskFailAllocation(0);out=(void*)1;
    assert(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,&out)==E_OUTOFMEMORY && !out && !aux_refs);
    texture_status=E_FAIL;texture_partial=TRUE;
    assert(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,&out)==E_FAIL && !out && !aux_refs);
    texture_status=S_OK;texture_partial=FALSE;view_status=E_ABORT;view_partial=TRUE;
    assert(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,&out)==E_ABORT && !out && !aux_refs && !view_refs);
    reset(1,0,26);assert(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,&out)==S_FALSE && !out && original_views==1);
    reset(2,1,62);metadata_status=E_NOINTERFACE;
    assert(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,&out)==S_FALSE && original_views==2);
    metadata_status=E_FAIL;
    assert(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,&out)==E_FAIL && !out);
    reset(2,1,62);source[8]|=0x40;assert(FM5SelectExpandedFMask(source,requested,&plan)==S_FALSE);
    reset(2,1,62);source[3]=2;assert(FM5SelectExpandedFMask(source,requested,&plan)==E_NOTIMPL);
    reset(2,1,62);requested[2]=1;assert(FM5SelectExpandedFMask(source,requested,&plan)==E_NOTIMPL);
    reset(2,1,62);requested[4]=1;assert(FM5SelectExpandedFMask(source,requested,&plan)==E_NOTIMPL);
    reset(2,1,62);source[0]=0;assert(FM5SelectExpandedFMask(source,requested,&plan)==E_INVALIDARG);
    reset(2,1,62);device_table[5]=NULL;
    assert(FAILED(FM5CreateLegacyShaderResourceView(&device,&source_object,requested,&out)) && !out);
    puts("PASS: six proven ordinary FMASK combinations, identity pixels/row/slice layout, native texture+SRV lifetime, allocation/create/view rollback, validation-only output, ordinary pass-through, unsupported EQAA/layout rejection");
    return 0;
}
