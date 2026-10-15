#include "fm5_dxgi_bridge.h"
#include <string.h>
#include "../audio-tls/resource-port/fm5_resource.h"
#include "../audio-tls/performance-device/fm5_performance_device.h"

static FM5GetSwapChainBuffer get_buffer;
static FM5CreateSwapChainForHwnd create_hwnd;
static FM5CreateSwapChainForCoreWindow create_core;
static const unsigned char hwnd_signature[32] = {
    0x48,0x89,0x5c,0x24,0x10,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,
    0x48,0x8d,0x6c,0x24,0xd0,0x48,0x81,0xec,0x30,0x01,0x00,0x00,0x48,0x8b,0x05,0x4d
};
static const unsigned char core_signature[32] = {
    0x48,0x83,0xec,0x78,0x48,0x8b,0x05,0x05,0x8b,0x2c,0x00,0x48,0x33,0xc4,0x48,0x89,
    0x44,0x24,0x60,0x4c,0x8b,0x94,0x24,0xa8,0x00,0x00,0x00,0x4d,0x85,0xc0,0x74,0x11
};

static HRESULT WINAPI for_hwnd(void *factory, void *device, HWND window,
    const void *description, const void *fullscreen, void *output, void **swapchain) {
    if (!create_hwnd) return E_UNEXPECTED;
    return create_hwnd(factory, FM5UnwrapPerformanceDevice(device), window,
        description, fullscreen, output, swapchain);
}

static HRESULT WINAPI for_core(void *factory, void *device, void *window,
    const void *description, void *output, void **swapchain) {
    if (!create_core) return E_UNEXPECTED;
    return create_core(factory, FM5UnwrapPerformanceDevice(device), window,
        description, output, swapchain);
}


static HRESULT WINAPI for_buffer(void *swapchain,UINT index,const void *iid,void **out) {
    if(!get_buffer)return E_UNEXPECTED;
    if(!out)return get_buffer(swapchain,index,iid,NULL);
    *out=NULL;
    void *resource=NULL;
    HRESULT hr=get_buffer(swapchain,index,iid,&resource);
    if(SUCCEEDED(hr) && resource) {
        HRESULT adopted=FM5AdoptLegacyTexture2D(resource);
        if(SUCCEEDED(adopted)) {*out=resource;return hr;}
        hr=adopted;
    } else if(SUCCEEDED(hr))hr=E_UNEXPECTED;
    if(resource) ((ULONG (WINAPI *)(void *))(*(void ***)resource)[2])(resource);
    return hr;
}

HRESULT FM5PatchFactoryVtable(void **table, UINT kind,
    const unsigned char *base, size_t size) {
    if (kind != 0xe && kind != 0xf) return E_INVALIDARG;
    if (!table || !base) return E_POINTER;
#ifndef FM5_DXGI_TEST
    if (base != (const unsigned char *)GetModuleHandleW(NULL))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
#endif
    if(kind==0xf) {
        static const unsigned char buffer_signature[32]={0x48,0x89,0x5c,0x24,0x10,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0x6c,0x24,0xd9,0x48,0x81,0xec,0x00,0x01,0x00,0x00,0x48,0x8b,0x05,0x5d};
        if(size<0x4f7c0+sizeof(buffer_signature) || table[10]!=base+0x4f7c0 ||
            memcmp(base+0x4f7c0,buffer_signature,sizeof(buffer_signature)))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
#ifndef FM5_DXGI_TEST
        get_buffer=(FM5GetSwapChainBuffer)(base+0x4f7c0);
#endif
        table[10]=(void *)for_buffer;return S_OK;
    }
    if (size < 0x4f730 + sizeof(core_signature) ||
        table[16] != base + 0x4eed0 || table[17] != base + 0x4f730 ||
        memcmp(base + 0x4eed0, hwnd_signature, sizeof(hwnd_signature)) ||
        memcmp(base + 0x4f730, core_signature, sizeof(core_signature)))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
#ifndef FM5_DXGI_TEST
    create_hwnd = (FM5CreateSwapChainForHwnd)(base + 0x4eed0);
    create_core = (FM5CreateSwapChainForCoreWindow)(base + 0x4f730);
#endif
    table[16] = (void *)for_hwnd;
    table[17] = (void *)for_core;
    return S_OK;
}

#ifdef FM5_DXGI_TEST
void FM5DXGITestGetBuffer(FM5GetSwapChainBuffer backend) {get_buffer=backend;}
void FM5DXGITestBackends(FM5CreateSwapChainForHwnd hwnd_backend,
    FM5CreateSwapChainForCoreWindow core_backend) {
    create_hwnd = hwnd_backend;
    create_core = core_backend;
}
#endif
