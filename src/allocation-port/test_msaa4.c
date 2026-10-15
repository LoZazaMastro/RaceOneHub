#include "fm5_msaa4.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

static void *expected_device, *expected_linkage;
static void **expected_output;
static const void *expected_code;
static SIZE_T expected_bytes;
static HRESULT expected_result;
static UINT calls,observer_calls;
static const void *observer_code;static SIZE_T observer_bytes;
static void observe(void *shader,const void *original,SIZE_T bytes) {
    assert(shader==(void*)(uintptr_t)0x123456789abcdef0);
    assert(original==observer_code && bytes==observer_bytes);++observer_calls;
}

static HRESULT WINAPI create(void *device,const void *code,SIZE_T bytes,void *linkage,void **out) {
    assert(device==expected_device && code==expected_code && bytes==expected_bytes);
    assert(linkage==expected_linkage && out==expected_output);
    if(out)*out=(void *)(uintptr_t)0x123456789abcdef0;
    ++calls;return expected_result;
}
static BYTE *load(const char *path,SIZE_T *bytes) {
    FILE *f=NULL;assert(!fopen_s(&f,path,"rb") && f);
    assert(!fseek(f,0,SEEK_END));long n=ftell(f);assert(n>0);rewind(f);
    BYTE *data=(BYTE*)malloc((SIZE_T)n);assert(data);
    assert(fread(data,1,(SIZE_T)n,f)==(SIZE_T)n);fclose(f);*bytes=(SIZE_T)n;return data;
}
int main(int argc,char **argv) {
    assert(argc==4);
    SIZE_T original_bytes,unknown_bytes,selected_bytes;
    BYTE *original=load(argv[1],&original_bytes),*unknown=load(argv[2],&unknown_bytes);
    const void *selected=NULL;
    assert(FM5SelectFourSampleResolve(original,original_bytes,2,&selected,&selected_bytes)==S_FALSE);
    assert(selected==original && selected_bytes==original_bytes);
    assert(FM5SelectFourSampleResolve(unknown,unknown_bytes,4,&selected,&selected_bytes)==S_OK);
    assert(FM5SelectFourSampleResolve(original,original_bytes,4,&selected,&selected_bytes)==S_OK);
    assert(selected!=original && selected_bytes==1004);
    const void *translation=selected;SIZE_T translation_bytes=selected_bytes;
    FILE *f=NULL;assert(!fopen_s(&f,argv[3],"wb") && f);
    assert(fwrite(selected,1,selected_bytes,f)==selected_bytes);fclose(f);
    UINT chunks;memcpy(&chunks,original+28,4);UINT refusals=0;
    for(UINT i=0;i<chunks;++i) {
        UINT offset,size;memcpy(&offset,original+32+4*i,4);memcpy(&size,original+offset+4,4);
        if(memcmp(original+offset,"RDEF",4) && memcmp(original+offset,"ISGN",4) &&
           memcmp(original+offset,"OSGN",4) && memcmp(original+offset,"SHEX",4))continue;
        for(UINT n=0;n<size;++n) {
            original[offset+8+n]^=1;
            assert(FM5SelectFourSampleResolve(original,original_bytes,4,&selected,&selected_bytes)!=S_OK);
            assert(selected==original && selected_bytes==original_bytes);
            original[offset+8+n]^=1;++refusals;
        }
    }
    BYTE *host=(BYTE*)calloc(1,0x319000);assert(host);
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)host;dos->e_magic=IMAGE_DOS_SIGNATURE;dos->e_lfanew=0x80;
    IMAGE_NT_HEADERS64 *pe=(IMAGE_NT_HEADERS64*)(host+0x80);pe->Signature=IMAGE_NT_SIGNATURE;
    pe->OptionalHeader.Magic=IMAGE_NT_OPTIONAL_HDR64_MAGIC;pe->OptionalHeader.SizeOfImage=0x319000;
    const UINT sites[]={0xe752a,0x130a4e,0x132310,0x1a7e60,0x1ab026};
    for(UINT i=0;i<_countof(sites);++i)memset(host+sites[i],0x90,2);
    memset(host+0xe7551,0x90,6);*(UINT*)(host+0x318ea0)=4;
    FM5SetMsaaTestHost(host);assert(FM5ForcedFourSampleMode());
    for(UINT mask=0;mask<16;++mask)assert(FM5MapNativeSampleMask(4,2,2,mask)==mask);
    assert(FM5MapNativeSampleMask(4,2,1,1)==0xa);
    assert(FM5MapNativeSampleMask(4,2,1,2)==5);
    assert(FM5MapNativeSampleMask(4,2,1,3)==15);
    assert(FM5MapNativeSampleMask(4,4,3,0xffffffffu)==0xffffffffu);
    assert(FM5MapNativeSampleMask(4,8,4,0)==0);
    BOOL partial_rejected=FALSE;
    __try { (void)FM5MapNativeSampleMask(4,4,3,1); }
    __except(GetExceptionCode()==0xe0f5000d?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) { partial_rejected=TRUE; }
    assert(partial_rejected);
    for(UINT i=0;i<_countof(sites);++i)for(UINT n=0;n<2;++n) {
        host[sites[i]+n]^=1;assert(!FM5ForcedFourSampleMode());host[sites[i]+n]^=1;
    }
    for(UINT n=0;n<6;++n){host[0xe7551+n]^=1;assert(!FM5ForcedFourSampleMode());host[0xe7551+n]^=1;}
    UINT texture[12]={1920,1080,1,1,26,2,1,0,0x28,0,0,0},srv[6]={62,4,0,1,0,0};
    FM5FMaskPlan plan;
    for(UINT fragments=2;fragments<=4;fragments+=2)for(UINT quality=1;quality<=2;++quality) {
        texture[5]=fragments;texture[6]=quality;
        assert(FM5SelectFourSampleFMask(texture,srv,&plan)==S_OK);
        assert(plan.fragments==4 && plan.coverage_samples==4 && plan.expanded_value==0xe4 && plan.format==62 && plan.element_bytes==1);
    }
    texture[5]=8;assert(FM5SelectFourSampleFMask(texture,srv,&plan)==E_NOTIMPL);
    FM5ConfigurePixelShaderObserver(observe);observer_code=original;observer_bytes=original_bytes;
    void *vtable[16]={0};vtable[15]=(void*)&create;void **object=vtable;
    expected_device=&object;expected_linkage=(void*)(uintptr_t)0xfedcba9876543210;
    void *output=NULL;expected_output=&output;expected_code=translation;expected_bytes=translation_bytes;
    for(UINT i=0;i<2;++i){expected_result=i?E_FAIL:S_OK;
        assert(FM5CreateLegacyPixelShader(&object,original,original_bytes,expected_linkage,&output)==expected_result);
        assert(output==(void*)(uintptr_t)0x123456789abcdef0);
    }
    *(UINT*)(host+0x318ea0)=2;assert(!FM5ForcedFourSampleMode());
    expected_code=original;expected_bytes=original_bytes;expected_result=S_FALSE;
    assert(FM5CreateLegacyPixelShader(&object,original,original_bytes,expected_linkage,&output)==S_FALSE);
    *(UINT*)(host+0x318ea0)=4;
    BYTE unrelated[32]={0};
    observer_code=unrelated;observer_bytes=sizeof(unrelated);
    expected_code=unrelated;expected_bytes=sizeof(unrelated);
    assert(FM5CreateLegacyPixelShader(&object,unrelated,sizeof(unrelated),expected_linkage,&output)==S_FALSE);
    expected_output=NULL;
    assert(FM5CreateLegacyPixelShader(&object,unrelated,sizeof(unrelated),expected_linkage,NULL)==S_FALSE);
    assert(calls==5);
    assert(observer_calls==3);FM5ConfigurePixelShaderObserver(NULL);
    printf("PASS: exact4chunk byteguard; %u independent shader mutations refused; native6site mode gate;4physical FMASK indices; exact5argument PixelShader forwarding/HR/output; unknownshader and2x unchanged\n",refusals);
    free(original);free(unknown);free(host);return 0;
}
