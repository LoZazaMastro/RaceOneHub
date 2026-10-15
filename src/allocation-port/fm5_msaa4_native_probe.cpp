#define WIN32_LEAN_AND_MEAN
#include "fm5_msaa4.h"
#include <d3d11.h>
#include <d3d12.h>
#include <string.h>
#include "fm5_resolve_four_samples.inc"

extern "C" HRESULT WINAPI FM5ProbeFourSampleDevice(void *native_device,
    UINT *physical_samples,UINT *physical_quality,UINT *created_pixel_shaders) {
    if (!native_device || !physical_samples || !physical_quality || !created_pixel_shaders) return E_POINTER;
    *physical_samples=*physical_quality=*created_pixel_shaders=0;
    if (!FM5ForcedFourSampleMode()) return S_FALSE;
    const BYTE *host=(const BYTE*)GetModuleHandleW(NULL);
    void **table=*(void***)native_device;
    static const BYTE texture_signature[]={0x48,0x8b,0xc4,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48};
    static const BYTE shader_signature[]={0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
    if(table[5]!=host+0xea000 || memcmp(table[5],texture_signature,sizeof(texture_signature)) ||
       table[15]!=host+0x664c0 || memcmp(table[15],shader_signature,sizeof(shader_signature)))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    ID3D11Device *device=(ID3D11Device*)native_device;
    D3D11_TEXTURE2D_DESC desc={};
    desc.Width=desc.Height=4;desc.MipLevels=desc.ArraySize=1;
    desc.Format=DXGI_FORMAT_R11G11B10_FLOAT;
    desc.SampleDesc.Count=2;desc.SampleDesc.Quality=2; /* genuine old EQAA request */
    desc.BindFlags=D3D11_BIND_RENDER_TARGET|D3D11_BIND_SHADER_RESOURCE;
    DWORD native_description[13]={};
    memcpy(native_description,&desc,sizeof(desc));
    typedef HRESULT (WINAPI *CreateTexture)(void *,const void *,const void *,void **);
    ID3D11Texture2D *texture=NULL;
    HRESULT status=((CreateTexture)table[5])(native_device,native_description,NULL,(void**)&texture);
    if(FAILED(status) || !texture) {
        if(texture)texture->Release();return FAILED(status)?status:E_UNEXPECTED;
    }
    /* Texture+40 owns an allocation wrapper, not a COM resource. The pinned
       native description helper e84e0 follows wrapper+10 to GetDesc+50. */
    if(*(const UINT*)((const BYTE*)texture+0x38)!=0xc) {
        texture->Release();return E_NOINTERFACE;
    }
    static const BYTE description_chain[]={0x48,0x8b,0x59,0x40,0x48,0x8d,0x54,0x24,0x20,0x48,0x8b,0xf9,0x48,0x8b,0x4b,0x10,0x48,0x8b,0x01,0xff,0x50,0x50};
    if(memcmp(host+0xe84f9,description_chain,sizeof(description_chain))) {
        texture->Release();return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    }
    BYTE *allocation=*(BYTE**)((BYTE*)texture+0x40);
    if(!allocation) {texture->Release();return E_UNEXPECTED;}
    ID3D12Resource *gpu=*(ID3D12Resource**)(allocation+0x10);
    if(!gpu) {texture->Release();return E_UNEXPECTED;}
    D3D12_RESOURCE_DESC physical=gpu->GetDesc();
    *physical_samples=physical.SampleDesc.Count;*physical_quality=physical.SampleDesc.Quality;
    if(physical.Dimension!=D3D12_RESOURCE_DIMENSION_TEXTURE2D || physical.Width!=4 ||
       physical.Height!=4 || physical.SampleDesc.Count!=4) {
        texture->Release();return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    }
    const void *bytecodes[]={fm5_resolve_four_samples,fm5_resolve_four_samples_tonemap};
    const SIZE_T sizes[]={sizeof(fm5_resolve_four_samples),sizeof(fm5_resolve_four_samples_tonemap)};
    for(UINT i=0;i<2;++i) {
        ID3D11PixelShader *shader=NULL;
        status=device->CreatePixelShader(bytecodes[i],sizes[i],NULL,&shader);
        if(FAILED(status) || !shader) {
            if(shader)shader->Release();texture->Release();return FAILED(status)?status:E_UNEXPECTED;
        }
        ++*created_pixel_shaders;shader->Release();
    }
    texture->Release();return S_OK;
}
