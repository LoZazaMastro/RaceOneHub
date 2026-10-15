#include <windows.h>
#include <d3d11.h>
#include <d3d11sdklayers.h>
#include <wrl/client.h>
#include <assert.h>
#include <stdio.h>
#include <vector>
extern "C" {
#include "fm5_performance_context.h"
}
using Microsoft::WRL::ComPtr;
struct Native { void **table; ULONG refs; };
static ID3D11DeviceContext *hardware;
static unsigned copies, resolves;
static ULONG release_native(void *p) { return --static_cast<Native *>(p)->refs; }
static HRESULT WINAPI description(void *resource, void *out) {
    ComPtr<ID3D11Texture2D> texture;
    HRESULT hr=static_cast<ID3D11Resource *>(resource)->QueryInterface(IID_PPV_ARGS(&texture));
    if(FAILED(hr)) return hr;
    D3D11_TEXTURE2D_DESC desc={}; texture->GetDesc(&desc);
    ZeroMemory(out,48); CopyMemory(out,&desc,sizeof(desc)); return S_OK;
}
static void copy(void *,void *dst,UINT di,UINT x,UINT y,UINT z,void *src,UINT si,const D3D11_BOX *box) {
    ++copies;
    hardware->CopySubresourceRegion(static_cast<ID3D11Resource *>(dst),di,x,y,z,
        static_cast<ID3D11Resource *>(src),si,box);
}
static void resolve(void *,void *dst,UINT di,void *src,UINT si,DXGI_FORMAT format) {
    ++resolves;
    hardware->ResolveSubresource(static_cast<ID3D11Resource *>(dst),di,
        static_cast<ID3D11Resource *>(src),si,format);
}
static void check_pixels(ID3D11Texture2D *texture,UINT subresource,UINT width,UINT height,UINT expected) {
    D3D11_MAPPED_SUBRESOURCE data={};
    assert(SUCCEEDED(hardware->Map(texture,subresource,D3D11_MAP_READ,0,&data)));
    for(UINT y=0;y<height;++y) for(UINT x=0;x<width;++x)
        assert(reinterpret_cast<const UINT *>(static_cast<const BYTE *>(data.pData)+y*data.RowPitch)[x]==expected);
    hardware->Unmap(texture,subresource);
}
int main() {
    ComPtr<ID3D11Device> device;
    ComPtr<ID3D11DeviceContext> context;
    D3D_FEATURE_LEVEL level;
    HRESULT hr=D3D11CreateDevice(nullptr,D3D_DRIVER_TYPE_HARDWARE,nullptr,D3D11_CREATE_DEVICE_DEBUG,
        nullptr,0,D3D11_SDK_VERSION,&device,&level,&context);
    if(FAILED(hr)) { fprintf(stderr,"Hardware debug device failed: %08lx\n",static_cast<unsigned long>(hr)); return 3; }
    hardware=context.Get();
    ComPtr<ID3D11InfoQueue> messages; assert(SUCCEEDED(device.As(&messages)));
    messages->ClearStoredMessages();
    void *table[270]={}; Native native={table,1}; void *proxy=nullptr;
    table[2]=reinterpret_cast<void *>(release_native);
    table[0x2e]=reinterpret_cast<void *>(copy); table[0x39]=reinterpret_cast<void *>(resolve);
    FM5ConfigureContextResolve(description);
    assert(SUCCEEDED(FM5WrapPerformanceContext(&native,&proxy)));
    using Resolve=void (*)(void *,void *,UINT,void *,UINT,DXGI_FORMAT);
    auto invoke=reinterpret_cast<Resolve>((*static_cast<void ***>(proxy))[0x3a]);

    D3D11_TEXTURE2D_DESC desc={}; desc.Width=desc.Height=4; desc.MipLevels=2; desc.ArraySize=2;
    desc.Format=DXGI_FORMAT_R8G8B8A8_UNORM; desc.SampleDesc.Count=1;
    desc.Usage=D3D11_USAGE_DEFAULT;
    UINT pixels[4][16]={}; D3D11_SUBRESOURCE_DATA initial[4]={};
    for(UINT i=0;i<4;++i) {
        for(auto &pixel:pixels[i]) pixel=0xff441100u+i*0x11;
        initial[i].pSysMem=pixels[i]; initial[i].SysMemPitch=(i%2?2:4)*4;
    }
    ComPtr<ID3D11Texture2D> single, staging;
    assert(SUCCEEDED(device->CreateTexture2D(&desc,initial,&single)));
    desc.Usage=D3D11_USAGE_STAGING; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&staging)));
    invoke(proxy,staging.Get(),1,single.Get(),3,static_cast<DXGI_FORMAT>(0xfeedabcd));
    check_pixels(staging.Get(),1,2,2,0xff441133);
    assert(copies==1 && resolves==0);

    UINT quality=0;
    assert(SUCCEEDED(device->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM,2,&quality)) && quality);
    desc.MipLevels=1; desc.Usage=D3D11_USAGE_DEFAULT; desc.CPUAccessFlags=0;
    desc.SampleDesc.Count=2; desc.BindFlags=D3D11_BIND_RENDER_TARGET;
    ComPtr<ID3D11Texture2D> multisample, resolved, resolved_staging;
    assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&multisample)));
    D3D11_RENDER_TARGET_VIEW_DESC view={}; view.Format=desc.Format;
    view.ViewDimension=D3D11_RTV_DIMENSION_TEXTURE2DMSARRAY;
    view.Texture2DMSArray.FirstArraySlice=1; view.Texture2DMSArray.ArraySize=1;
    ComPtr<ID3D11RenderTargetView> target;
    assert(SUCCEEDED(device->CreateRenderTargetView(multisample.Get(),&view,&target)));
    const FLOAT green[]={0,1,0,1}; hardware->ClearRenderTargetView(target.Get(),green);
    desc.SampleDesc.Count=1; desc.BindFlags=0;
    assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&resolved)));
    desc.Usage=D3D11_USAGE_STAGING; desc.CPUAccessFlags=D3D11_CPU_ACCESS_READ;
    assert(SUCCEEDED(device->CreateTexture2D(&desc,nullptr,&resolved_staging)));
    invoke(proxy,resolved.Get(),1,multisample.Get(),1,DXGI_FORMAT_R8G8B8A8_UNORM);
    hardware->CopySubresourceRegion(resolved_staging.Get(),1,0,0,0,resolved.Get(),1,nullptr);
    check_pixels(resolved_staging.Get(),1,4,4,0xff00ff00);
    assert(copies==1 && resolves==1);
    UINT errors=0;
    for(UINT64 i=0;i<messages->GetNumStoredMessages();++i) {
        SIZE_T bytes=0; assert(SUCCEEDED(messages->GetMessage(i,nullptr,&bytes)));
        std::vector<BYTE> storage(bytes); auto message=reinterpret_cast<D3D11_MESSAGE *>(storage.data());
        assert(SUCCEEDED(messages->GetMessage(i,message,&bytes)));
        if(message->Severity==D3D11_MESSAGE_SEVERITY_ERROR || message->Severity==D3D11_MESSAGE_SEVERITY_CORRUPTION) {
            ++errors; fprintf(stderr,"D3D11 error %u: %s\n",message->ID,message->pDescription);
        }
    }
    assert(!errors);
    assert(reinterpret_cast<ULONG(*)(void*)>((*static_cast<void***>(proxy))[2])(proxy)==0 && !native.refs);
    puts("PASS: real hardware single-sample array/mip copy with ignored Format; 2x MSAA resolve array slice; complete pixel readback; zero debug-layer errors/corruption");
    return 0;
}
