#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4201)
#include <d3d12.h>
#pragma warning(pop)
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "allocation-port/fm5_msaa4.h"

extern "C" HRESULT WINAPI FM5GetLegacyTexture2DDescription(void *, void *);

extern "C" int FM5RunNativeMsaaProbe(void *device) {
    typedef HRESULT (WINAPI *CreateTexture)(void *,const void *,const void *,void **);
    typedef HRESULT (WINAPI *CreatePixel)(void *,const void *,SIZE_T,void *,void **);
    typedef ULONG (WINAPI *Release)(void *);
    UINT description[12] = {32,32,1,1,28,2,2,0,0x28,0,0,0};
    UINT legacy[12] = {};
    void *texture = NULL, *shader = NULL;
    ID3D12Resource *physical = NULL;
    HRESULT hr = E_FAIL;
    D3D12_RESOURCE_DESC actual = {};
    bool passed = false;
    UINT created_shaders = 0;
    UINT requested_samples=FM5ForcedSampleCount();
    if (!device || (requested_samples!=4 && requested_samples!=8)) goto done;
    fprintf(stderr,"{\"event\":\"fm5_msaa_probe_stage\",\"stage\":\"texture_create\"}\n"); fflush(stderr);
    hr = ((CreateTexture)(*(void ***)device)[5])(device,description,NULL,&texture);
    if (FAILED(hr) || !texture) goto done;
    fprintf(stderr,"{\"event\":\"fm5_msaa_probe_stage\",\"stage\":\"legacy_desc\"}\n"); fflush(stderr);
    hr = FM5GetLegacyTexture2DDescription(texture,legacy);
    if (FAILED(hr) || legacy[5] != 2 || legacy[6] != 2) goto done;
    /* Pinned native e84e0/e8505 dereferences Texture+40 allocation holder,
       then holder+10 to reach ID3D12Resource. The holder itself is not COM. */
    {
        void *allocation = NULL;
        IUnknown *underlying = NULL;
        if (*(const UINT *)((const BYTE *)texture + 0x38) != 0xc) goto done;
        memcpy(&allocation,(const BYTE *)texture + 0x40,sizeof(allocation));
        if (!allocation) goto done;
        memcpy(&underlying,(const BYTE *)allocation + 0x10,sizeof(underlying));
        if (!underlying) goto done;
        fprintf(stderr,"{\"event\":\"fm5_msaa_probe_stage\",\"stage\":\"physical_query\"}\n"); fflush(stderr);
        hr = underlying->QueryInterface(__uuidof(ID3D12Resource),(void **)&physical);
        if (FAILED(hr) || !physical) goto done;
        actual = physical->GetDesc();
        fprintf(stderr,"{\"event\":\"fm5_msaa_probe_stage\",\"stage\":\"physical_desc\",\"samples\":%u}\n",actual.SampleDesc.Count); fflush(stderr);
    }
    if (actual.Width != 32 || actual.Height != 32 || actual.SampleDesc.Count != requested_samples ||
        (actual.SampleDesc.Quality != 0 && actual.SampleDesc.Quality != D3D12_STANDARD_MULTISAMPLE_PATTERN)) goto done;
    {
        ID3D12Device *gpu_device=NULL;
        hr=physical->GetDevice(__uuidof(ID3D12Device),(void**)&gpu_device);
        if(FAILED(hr)||!gpu_device)goto done;
        const DXGI_FORMAT formats[]={DXGI_FORMAT_R11G11B10_FLOAT,DXGI_FORMAT_D32_FLOAT_S8X24_UINT,
                                     DXGI_FORMAT_R16G16B16A16_FLOAT,DXGI_FORMAT_R8G8B8A8_UNORM};
        bool supported=true;
        for(UINT i=0;i<_countof(formats);++i) {
            D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS query={formats[i],requested_samples,D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE,0};
            HRESULT capability=gpu_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,&query,sizeof(query));
            fprintf(stderr,"{\"event\":\"fm5_msaa_probe_capability\",\"format\":%u,\"samples\":%u,\"quality_levels\":%u,\"hr\":\"0x%08lx\"}\n",(UINT)formats[i],requested_samples,query.NumQualityLevels,(unsigned long)capability);
            if(FAILED(capability)||!query.NumQualityLevels){supported=false;hr=FAILED(capability)?capability:DXGI_ERROR_UNSUPPORTED;}
        }
        gpu_device->Release();
        if(!supported)goto done;
    }
    {
        const BYTE *guest = (const BYTE *)(uintptr_t)0x140000000;
        MEMORY_BASIC_INFORMATION region = {};
        if (!VirtualQuery(guest,&region,sizeof(region)) || region.State != MEM_COMMIT ||
            ((const IMAGE_DOS_HEADER *)guest)->e_magic != IMAGE_DOS_SIGNATURE) goto done;
        static const UINT shader_rvas[] = {0x11d41d0,0x11d9180,0x14a6140,0x14ab738,
                                         0x171a050,0x171ff0c,0x18a68b0,0x18aceb0};
        static const UINT shader_sizes[] = {51120,2468,56312,3564,64700,5808,70656,7668};
        for (UINT i = 0; i < _countof(shader_rvas); ++i) {
            const BYTE *original = guest + shader_rvas[i];
            UINT size = 0;
            if (!VirtualQuery(original,&region,sizeof(region)) || region.State != MEM_COMMIT ||
                memcmp(original,"DXBC",4)) goto done;
            memcpy(&size,original + 24,4);
            if (size != shader_sizes[i]) goto done;
            hr = ((CreatePixel)(*(void ***)device)[15])(device,original,size,NULL,&shader);
            if (FAILED(hr) || !shader) goto done;
            ++created_shaders;
            ((Release)(*(void ***)shader)[2])(shader); shader = NULL;
        }
    }
    passed = true;
done:
    if (shader) ((Release)(*(void ***)shader)[2])(shader);
    if (physical) physical->Release();
    if (texture) ((Release)(*(void ***)texture)[2])(texture);
    fprintf(stderr,"{\"event\":\"fm5_native_msaa_probe\",\"passed\":%s,\"hr\":\"0x%08lx\","
        "\"physical_samples\":%u,\"physical_quality\":%u,\"legacy_samples\":%u,\"legacy_quality\":%u,\"created_shaders\":%u}\n",
        passed ? "true" : "false",(unsigned long)hr,actual.SampleDesc.Count,
        actual.SampleDesc.Quality,legacy[5],legacy[6],created_shaders);
    fflush(stderr);
    return passed ? 1 : 0;
}
