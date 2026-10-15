#include "fm5_resource.h"
#include <stdio.h>
#include <string.h>

typedef HRESULT (WINAPI *CreateFn)(void *,const void *,const void *,void **);
typedef HRESULT (WINAPI *PlacementFn)(void *,const void *,UINT,UINT,void *,void **);
typedef ULONG (WINAPI *RefFn)(void *);
typedef void (WINAPI *DescFn)(void *,void *);
typedef union Call { void *pointer;CreateFn create;PlacementFn placement;RefFn release;DescFn get; } Call;
typedef struct Fake {void **table;LONG refs;UINT kind;} Fake;
static void *device_table[66],*resource_table[13];
static Fake device={device_table,1,0},resource={resource_table,0,0};
static UINT sizes[4]={28,36,48,40},misc_offsets[4]={16,28,40,32};
static DWORD captured[13],expected_prefix[12];
static UINT current_slot,create_calls,get_calls;
static const void *last_initial;
static HRESULT native_result=S_OK;
static BOOL return_partial,return_null;
static UINT captured_a,captured_b;
static void *captured_placement;
static ULONG WINAPI release(void *self) { return (ULONG)InterlockedDecrement(&((Fake *)self)->refs); }
static HRESULT WINAPI create(void *self,const void *description,const void *initial,void **out) {
    if(self!=&device) return E_INVALIDARG;
    ++create_calls;last_initial=initial;
    UINT i=current_slot-3;memset(captured,0xee,sizeof(captured));
    memcpy(captured,description,sizes[i]+4);
    if(!out) return native_result==S_OK?S_FALSE:native_result;
    *out=NULL;
    if((SUCCEEDED(native_result) && !return_null) || return_partial) {++resource.refs;resource.kind=i;*out=&resource;}
    return native_result;
}
static void WINAPI getdesc(void *self,void *out) {
    Fake *r=(Fake *)self;++get_calls;
    memcpy(out,expected_prefix,sizes[r->kind]-4);
}
static HRESULT WINAPI placement(void *self,const void *description,UINT a,UINT b,void *address,void **out) {
    if(self!=&device)return E_INVALIDARG;
    ++create_calls;captured_a=a;captured_b=b;captured_placement=address;memcpy(captured,description,52);
    if(!out)return E_INVALIDARG;
    *out=NULL;
    if((SUCCEEDED(native_result) && !return_null) || return_partial) {++resource.refs;resource.kind=2;*out=&resource;}
    return native_result;
}
static HRESULT WINAPI placement_buffer(void *self,const void *description,const void *address,void **out) {
    if(self!=&device)return E_INVALIDARG;
    ++create_calls;captured_placement=(void *)address;
    memset(captured,0xee,sizeof(captured));memcpy(captured,description,24);
    if(!out)return E_INVALIDARG;
    *out=NULL;
    if((SUCCEEDED(native_result) && !return_null) || return_partial) {++resource.refs;resource.kind=0;*out=&resource;}
    return native_result;
}
static HRESULT WINAPI placement3d(void *self,const void *description,UINT a,UINT b,void *address,void **out) {
    if(self!=&device)return E_INVALIDARG;
    ++create_calls;captured_a=a;captured_b=b;captured_placement=address;
    memset(captured,0xee,sizeof(captured));memcpy(captured,description,44);
    if(!out)return E_INVALIDARG;
    *out=NULL;
    if((SUCCEEDED(native_result) && !return_null) || return_partial){++resource.refs;resource.kind=3;*out=&resource;}
    return native_result;
}
static unsigned checks;
#define CHECK(x) do {++checks;if(!(x)) {printf("failed_line=%u\n",__LINE__);return 1;}}while(0)
int main(void) {
    Call call;call.create=create;for(UINT i=3;i<=6;++i) device_table[i]=call.pointer;
    call.placement=placement;device_table[64]=call.pointer;
    call.placement=placement3d;device_table[65]=call.pointer;
    call.create=placement_buffer;device_table[62]=call.pointer;
    call.release=release;resource_table[2]=call.pointer;call.get=getdesc;resource_table[12]=call.pointer;
    void *out=(void *)1;DWORD desc[12];
    CHECK(FM5CreateLegacyResource(NULL,3,desc,NULL,&out)==E_INVALIDARG && !out);
    CHECK(FM5CreateLegacyResource(&device,7,desc,NULL,&out)==E_INVALIDARG && !out);
    CHECK(FM5CreateLegacyResource(&device,3,NULL,NULL,&out)==E_INVALIDARG && !out);
    CHECK(FM5MetadataGetDesc(&resource,desc,sizeof(desc))==HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    CHECK(FM5MetadataGetDesc(NULL,desc,sizeof(desc))==E_POINTER);
    CHECK(FM5GetLegacyTexture2DDescription(&resource,desc)==HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
    CHECK(FM5GetLegacyTexture2DDescription(NULL,desc)==E_POINTER);
    CHECK(FM5GetLegacyTexture2DDescription(&resource,NULL)==E_POINTER);
    CHECK(FM5ResourceMetadataCount()==0);
    for(UINT i=0;i<4;++i) {
        current_slot=i+3;UINT n=sizes[i],prefix=n-4;
        for(UINT j=0;j<12;++j) {desc[j]=0x12340000+j;expected_prefix[j]=0xaabb0000+j;}
        desc[misc_offsets[i]/4]=0;
        desc[n/4-1]=0x98765432;
        if(i==2) {desc[5]=8;desc[6]=3;expected_prefix[5]=4;expected_prefix[6]=0;}
        DWORD expected_legacy[12];memcpy(expected_legacy,expected_prefix,sizeof(expected_legacy));
        if(i==2) {expected_legacy[5]=desc[5];expected_legacy[6]=desc[6];}
        DWORD original[12];memcpy(original,desc,sizeof(desc));
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,&i,NULL)==S_FALSE && FM5ResourceMetadataCount()==0 && resource.refs==0 && captured[n/4]==0 && last_initial==&i);
        native_result=E_INVALIDARG;
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,NULL)==E_INVALIDARG && FM5ResourceMetadataCount()==0);
        native_result=S_OK;
        FM5ResourceFailAllocation(0);
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,NULL)==S_FALSE && FM5ResourceMetadataCount()==0 && resource.refs==0);
        FM5ResourceFailAllocation(-1);
        FM5ResourceFailAllocation(0);UINT old_calls=create_calls;
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,&i,&out)==E_OUTOFMEMORY && !out && create_calls==old_calls);
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,&i,&out)==S_OK && out==&resource && resource.refs==1);
        CHECK(!memcmp(desc,original,sizeof(desc)) && !memcmp(captured,desc,n) && captured[n/4]==0 && last_initial==&i);
        CHECK(FM5ResourceMetadataCount()==1);
        CHECK(FM5AdoptLegacyTexture2D(&resource)==(i==2?S_FALSE:E_NOINTERFACE) && FM5ResourceMetadataCount()==1 && resource.refs==1);
        BYTE result[64];memset(result,0xcc,sizeof(result));UINT old_get=get_calls;
        CHECK(FM5MetadataGetDesc(&resource,result+4,n-1)==HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) && get_calls==old_get && result[4]==0xcc);
        CHECK(FM5MetadataGetDesc(&resource,result+4,n)==S_OK && !memcmp(result+4,expected_legacy,prefix));
        CHECK(!memcmp(result+4+prefix,&desc[n/4-1],4) && result[3]==0xcc && result[n+4]==0xcc);
        memset(result,0xcc,sizeof(result));FM5LegacyGetDescription(&resource,result+4);
        CHECK(!memcmp(result+4,expected_legacy,prefix) && result[n+4]==0xcc);
        UINT typed_before=get_calls;memset(result,0xcc,sizeof(result));
        HRESULT typed=FM5GetLegacyTexture2DDescription(&resource,result+4);
        if(i==2) CHECK(typed==S_OK && !memcmp(result+4,expected_legacy,44) && *(DWORD *)(result+48)==desc[11] && result[3]==0xcc && result[52]==0xcc);
        else CHECK(typed==E_NOINTERFACE && get_calls==typed_before && result[4]==0xcc);
        out=(void *)1;CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,&out)==HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) && !out && resource.refs==1 && FM5ResourceMetadataCount()==1);
        FM5ForgetLegacyResource(&resource);CHECK(FM5ResourceMetadataCount()==0 && resource.refs==1);
        CHECK(FM5MetadataGetDesc(&resource,result,sizeof(result))==HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
        CHECK(FM5GetLegacyTexture2DDescription(&resource,result)==HRESULT_FROM_WIN32(ERROR_NOT_FOUND));
        FM5ForgetLegacyResource(&resource);release(&resource);CHECK(resource.refs==0);
        desc[n/4-1]=0x1000;
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,&out)==S_OK && out==&resource);
        CHECK(FM5MetadataGetDesc(&resource,result,sizeof(result))==S_OK && !memcmp(result+prefix,&desc[n/4-1],4));
        FM5ForgetLegacyResource(out);release(out);
        desc[misc_offsets[i]/4]=0x20000;desc[n/4-1]=0xffffffff;old_calls=create_calls;
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,&out)==E_NOTIMPL && !out && old_calls==create_calls);
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,NULL)==S_FALSE && captured[n/4-1]==0xffffffff && captured[n/4]==0 && FM5ResourceMetadataCount()==0);
        desc[n/4-1]=0x10000;
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,&out)==S_OK && captured[n/4-1]==0x10000 && captured[n/4]==0);
        FM5ForgetLegacyResource(out);release(out);
        native_result=E_FAIL;
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,&out)==E_FAIL && !out && FM5ResourceMetadataCount()==0);
        return_partial=TRUE;
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,&out)==E_FAIL && !out && resource.refs==0 && FM5ResourceMetadataCount()==0);
        return_partial=FALSE;native_result=S_OK;return_null=TRUE;
        CHECK(FM5CreateLegacyResource(&device,current_slot,desc,NULL,&out)==E_UNEXPECTED && !out && FM5ResourceMetadataCount()==0);
        return_null=FALSE;
    }
    DWORD buffer_desc[7]={144,1,4,0,0,0,0xdeadbeef};
    DWORD original_buffer[7];memcpy(original_buffer,buffer_desc,28);
    void *buffer_address=(void *)(uintptr_t)0xa0d790100;
    UINT buffer_calls=create_calls;
    FM5ResourceFailAllocation(0);
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,&out)==E_OUTOFMEMORY && !out && create_calls==buffer_calls);
    void *buffer_method=device_table[62];device_table[62]=NULL;
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,&out)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && !out && create_calls==buffer_calls && FM5ResourceMetadataCount()==0);
    device_table[62]=buffer_method;
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,&out)==S_OK && out==&resource && resource.refs==1);
    CHECK(!memcmp(buffer_desc,original_buffer,28) && !memcmp(captured,buffer_desc,24) && captured[6]==0xeeeeeeee && captured_placement==buffer_address);
    memcpy(expected_prefix,buffer_desc,24);expected_prefix[4]|=0x40000000;
    DWORD buffer_result[9];memset(buffer_result,0xcc,sizeof(buffer_result));
    CHECK(FM5MetadataGetDesc(&resource,buffer_result+1,28)==S_OK && !memcmp(buffer_result+1,buffer_desc,24) && buffer_result[7]==0 && buffer_result[0]==0xcccccccc && buffer_result[8]==0xcccccccc);
    CHECK(FM5GetLegacyTexture2DDescription(&resource,buffer_result)==E_NOINTERFACE);
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,&out)==HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) && !out && resource.refs==1 && FM5ResourceMetadataCount()==1);
    FM5ForgetLegacyResource(&resource);release(&resource);
    native_result=E_FAIL;
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,&out)==E_FAIL && !out && FM5ResourceMetadataCount()==0);
    return_partial=TRUE;
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,&out)==E_FAIL && !out && resource.refs==0 && FM5ResourceMetadataCount()==0);
    return_partial=FALSE;native_result=S_OK;return_null=TRUE;
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,&out)==E_UNEXPECTED && !out && FM5ResourceMetadataCount()==0);
    return_null=FALSE;native_result=S_FALSE;
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,&out)==S_FALSE && out==&resource && resource.refs==1);
    FM5ForgetLegacyResource(out);release(out);native_result=S_OK;
    buffer_calls=create_calls;
    CHECK(FM5CreateLegacyPlacementBuffer(&device,buffer_desc,buffer_address,NULL)==E_INVALIDARG && create_calls==buffer_calls);
    CHECK(FM5CreateLegacyPlacementBuffer(NULL,buffer_desc,buffer_address,&out)==E_INVALIDARG && !out);
    CHECK(FM5CreateLegacyPlacementBuffer(&device,NULL,buffer_address,&out)==E_INVALIDARG && !out);
    CHECK(FM5ResourceMetadataCount()==0 && resource.refs==0);
    DWORD placement_desc[12]={1920,1080,1,1,26,8,3,0,0x28,0,0x20000,0xdeadbeef};
    DWORD original_placement[12];memcpy(original_placement,placement_desc,48);
    memcpy(expected_prefix,placement_desc,44);expected_prefix[10]|=0x40000000;
    expected_prefix[5]=4;expected_prefix[6]=0;
    UINT packing_a=0x123456ab,packing_b=0x00cdef01,packed=0xabcdef01;
    void *placement_address=(void *)(uintptr_t)0x123456789abcdef0;
    UINT old_calls=create_calls;
    void *saved_method=device_table[64];device_table[64]=NULL;
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,packing_a,packing_b,placement_address,&out)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && !out && create_calls==old_calls);
    device_table[64]=saved_method;
    FM5ResourceFailAllocation(0);
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,packing_a,packing_b,placement_address,&out)==E_OUTOFMEMORY && !out && create_calls==old_calls);
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,packing_a,packing_b,placement_address,&out)==S_OK && out==&resource && resource.refs==1);
    CHECK(!memcmp(placement_desc,original_placement,48) && !memcmp(captured,placement_desc,44) && captured[11]==0 && captured[12]==0 && captured_a==packing_a && captured_b==packing_b && captured_placement==placement_address);
    BYTE placement_result[56];memset(placement_result,0xcc,sizeof(placement_result));
    CHECK(FM5MetadataGetDesc(&resource,placement_result+4,48)==S_OK && !memcmp(placement_result+4,placement_desc,44) && !memcmp(placement_result+48,&packed,4) && placement_result[3]==0xcc && placement_result[52]==0xcc);
    memset(placement_result,0xcc,sizeof(placement_result));FM5LegacyGetDescription(&resource,placement_result+4);
    CHECK(!memcmp(placement_result+4,placement_desc,44) && !memcmp(placement_result+48,&packed,4) && placement_result[52]==0xcc);
    memset(placement_result,0xcc,sizeof(placement_result));
    CHECK(FM5GetLegacyTexture2DDescription(&resource,placement_result+4)==S_OK && !memcmp(placement_result+4,placement_desc,44) && !memcmp(placement_result+48,&packed,4) && placement_result[3]==0xcc && placement_result[52]==0xcc);
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,packing_a,packing_b,placement_address,&out)==HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) && !out && resource.refs==1);
    FM5ForgetLegacyResource(&resource);release(&resource);CHECK(FM5ResourceMetadataCount()==0 && resource.refs==0);
    FM5ResourceFailAllocation(0);old_calls=create_calls;
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,packing_a,packing_b,placement_address,NULL)==E_INVALIDARG && create_calls==old_calls+1 && FM5ResourceMetadataCount()==0);
    FM5ResourceFailAllocation(-1);
    native_result=E_FAIL;
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,packing_a,packing_b,placement_address,&out)==E_FAIL && !out && FM5ResourceMetadataCount()==0);
    return_partial=TRUE;
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,packing_a,packing_b,placement_address,&out)==E_FAIL && !out && resource.refs==0);
    return_partial=FALSE;native_result=S_OK;return_null=TRUE;
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,packing_a,packing_b,placement_address,&out)==E_UNEXPECTED && !out && FM5ResourceMetadataCount()==0);
    return_null=FALSE;
    CHECK(FM5CreateLegacyPlacementTexture2D(NULL,placement_desc,packing_a,packing_b,placement_address,&out)==E_INVALIDARG && !out);
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,NULL,packing_a,packing_b,placement_address,&out)==E_INVALIDARG && !out);
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,0xe14,0,placement_address,&out)==S_OK && out==&resource);
    CHECK(FM5MetadataGetDesc(&resource,placement_result,48)==S_OK && *(DWORD *)(placement_result+44)==0x14000000);
    FM5ForgetLegacyResource(out);release(out);
    DWORD diagnostic_desc[12]={64,64,7,1,99,1,0,0,8,0,0,0};
    for(UINT i=0;i<20;++i) CHECK(FM5CreateLegacyPlacementTexture2D(&device,diagnostic_desc,0xe,0,(void *)(uintptr_t)0x100008000,NULL)==E_INVALIDARG);
    CHECK(FM5CreateLegacyPlacementTexture2D(&device,placement_desc,0xffffffff,0xffffffff,placement_address,&out)==S_OK && out==&resource && captured_a==0xffffffff && captured_b==0xffffffff);
    CHECK(FM5MetadataGetDesc(&resource,placement_result,48)==S_OK && *(DWORD *)(placement_result+44)==0xffffffff);
    FM5ForgetLegacyResource(out);release(out);
    DWORD desc3d[10]={128,128,128,8,80,0,8,0,0,0xdeadbeef};
    DWORD original3d[10];memcpy(original3d,desc3d,sizeof(desc3d));
    memcpy(expected_prefix,desc3d,36);expected_prefix[8]|=0x40000000;
    old_calls=create_calls;void *saved3d=device_table[65];device_table[65]=NULL;
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,13,0,placement_address,&out)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && !out && create_calls==old_calls);
    device_table[65]=saved3d;FM5ResourceFailAllocation(0);
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,13,0,placement_address,&out)==E_OUTOFMEMORY && !out && create_calls==old_calls);
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,13,0,placement_address,&out)==S_OK && out==&resource && resource.refs==1);
    CHECK(!memcmp(desc3d,original3d,sizeof(desc3d)) && !memcmp(captured,desc3d,36) && captured[9]==0 && captured[10]==0 && captured_a==13 && captured_b==0 && captured_placement==placement_address);
    BYTE result3d[48];memset(result3d,0xcc,sizeof(result3d));DWORD extra3d=0x0d000000;
    CHECK(FM5MetadataGetDesc(&resource,result3d+4,40)==S_OK && !memcmp(result3d+4,desc3d,36) && !memcmp(result3d+40,&extra3d,4) && result3d[3]==0xcc && result3d[44]==0xcc);
    memset(result3d,0xcc,sizeof(result3d));FM5LegacyGetDescription(&resource,result3d+4);
    CHECK(!memcmp(result3d+4,desc3d,36) && !memcmp(result3d+40,&extra3d,4) && result3d[3]==0xcc && result3d[44]==0xcc);
    CHECK(FM5GetLegacyTexture2DDescription(&resource,result3d)==E_NOINTERFACE);
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,13,0,placement_address,&out)==HRESULT_FROM_WIN32(ERROR_ALREADY_EXISTS) && !out && resource.refs==1 && FM5ResourceMetadataCount()==1);
    FM5ForgetLegacyResource(&resource);release(&resource);CHECK(FM5ResourceMetadataCount()==0 && resource.refs==0);
    old_calls=create_calls;FM5ResourceFailAllocation(0);
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,13,0,placement_address,NULL)==E_INVALIDARG && create_calls==old_calls+1 && FM5ResourceMetadataCount()==0);
    FM5ResourceFailAllocation(-1);native_result=E_FAIL;
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,13,0,placement_address,&out)==E_FAIL && !out && FM5ResourceMetadataCount()==0);
    return_partial=TRUE;
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,13,0,placement_address,&out)==E_FAIL && !out && resource.refs==0 && FM5ResourceMetadataCount()==0);
    return_partial=FALSE;native_result=S_OK;return_null=TRUE;
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,13,0,placement_address,&out)==E_UNEXPECTED && !out && FM5ResourceMetadataCount()==0);
    return_null=FALSE;
    CHECK(FM5CreateLegacyPlacementTexture3D(NULL,desc3d,13,0,placement_address,&out)==E_INVALIDARG && !out);
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,NULL,13,0,placement_address,&out)==E_INVALIDARG && !out);
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,0x123456ab,0x00cdef01,placement_address,&out)==S_OK && captured_a==0x123456ab && captured_b==0x00cdef01);
    CHECK(FM5MetadataGetDesc(&resource,result3d,40)==S_OK && *(DWORD *)(result3d+36)==0xabcdef01);
    FM5ForgetLegacyResource(out);release(out);native_result=S_FALSE;
    CHECK(FM5CreateLegacyPlacementTexture3D(&device,desc3d,0xffffffff,0xffffffff,placement_address,&out)==S_FALSE && out==&resource);
    CHECK(FM5MetadataGetDesc(&resource,result3d,40)==S_OK && *(DWORD *)(result3d+36)==0xffffffff);
    FM5ForgetLegacyResource(out);release(out);native_result=S_OK;
    BOOL rejected=FALSE;
    __try {FM5LegacyGetDescription(&resource,desc);}
    __except(GetExceptionCode()==FM5_E_RESOURCE_METADATA ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {rejected=TRUE;}
    CHECK(rejected && FM5ResourceMetadataCount()==0 && resource.refs==0);
    FM5ForgetLegacyResource(NULL);
    printf("{\"passed\":true,\"checks\":%u,\"descriptor_kinds\":4,\"placement_texture2d\":true,\"native_create_calls\":%u,\"native_getdesc_calls\":%u,\"metadata_remaining\":0,\"scope\":\"CPU metadata, typed descriptor conversion, HRESULT and lifetime contracts; recorder backend\"}\n",checks,create_calls,get_calls);
    return 0;
}
