#include "fm5_dxgi_bridge.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

HRESULT WINAPI FM5AdoptLegacyTexture2D(void *resource) {(void)resource;assert(!"unexpected GetBuffer adoption");return E_UNEXPECTED;}
static unsigned unwrap_calls, native_calls;
static int proxy, native_device, factory, description, fullscreen, restricted, core_window;
static void *expected_device, *returned_object;
static HWND expected_window;
static HRESULT expected_status;

void *WINAPI FM5UnwrapPerformanceDevice(void *candidate) {
    ++unwrap_calls;
    return candidate == &proxy ? &native_device : candidate;
}
static HRESULT WINAPI hwnd_call(void *self, void *device, HWND window,
    const void *desc, const void *full, void *output, void **out) {
    assert(self == &factory && device == expected_device && window == expected_window);
    assert(desc == &description && full == &fullscreen && output == &restricted);
    ++native_calls;
    if (!out) return E_POINTER;
    *out = returned_object;
    return expected_status;
}
static HRESULT WINAPI core_call(void *self, void *device, void *window,
    const void *desc, void *output, void **out) {
    assert(self == &factory && device == expected_device && window == &core_window);
    assert(desc == &description && output == &restricted);
    ++native_calls;
    if (!out) return E_POINTER;
    *out = returned_object;
    return expected_status;
}
static const unsigned char hwnd_bytes[] = {
    0x48,0x89,0x5c,0x24,0x10,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,
    0x48,0x8d,0x6c,0x24,0xd0,0x48,0x81,0xec,0x30,0x01,0x00,0x00,0x48,0x8b,0x05,0x4d
};
static const unsigned char core_bytes[] = {
    0x48,0x83,0xec,0x78,0x48,0x8b,0x05,0x05,0x8b,0x2c,0x00,0x48,0x33,0xc4,0x48,0x89,
    0x44,0x24,0x60,0x4c,0x8b,0x94,0x24,0xa8,0x00,0x00,0x00,0x4d,0x85,0xc0,0x74,0x11
};
int main(void) {
    const size_t size = 0x50000;
    unsigned char *image = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, size);
    void *table[270], *before[270], *out;
    unsigned i, pass;
    assert(image);
    memcpy(image + 0x4eed0, hwnd_bytes, sizeof(hwnd_bytes));
    memcpy(image + 0x4f730, core_bytes, sizeof(core_bytes));
    for (i=0; i<270; ++i) table[i]=(void *)(ULONG_PTR)(i+1);
    table[16]=image+0x4eed0; table[17]=image+0x4f730;
    memcpy(before, table, sizeof(table));
    assert(FAILED(FM5PatchFactoryVtable(table, 0xf, image, size)));
    assert(!memcmp(table,before,sizeof(table)));
    assert(FM5PatchFactoryVtable(NULL,0xe,image,size)==E_POINTER);
    assert(FM5PatchFactoryVtable(table,0xe,NULL,size)==E_POINTER);
    assert(FAILED(FM5PatchFactoryVtable(table,0xe,image,0x4f74f)));
    assert(!memcmp(table,before,sizeof(table)));
    for(i=0;i<64;++i) {
        size_t address=(i<32?0x4eed0:0x4f730)+(i%32);
        image[address]^=1;
        assert(FAILED(FM5PatchFactoryVtable(table,0xe,image,size)));
        assert(!memcmp(table,before,sizeof(table)));
        image[address]^=1;
    }
    table[16]=NULL;
    assert(FAILED(FM5PatchFactoryVtable(table,0xe,image,size)) && table[17]==before[17]);
    memcpy(table,before,sizeof(table)); table[17]=NULL;
    assert(FAILED(FM5PatchFactoryVtable(table,0xe,image,size)) && table[16]==before[16]);
    memcpy(table,before,sizeof(table));
    assert(FM5PatchFactoryVtable(table,0xe,image,size)==S_OK);
    for(i=0;i<270;++i) if(i!=16 && i!=17) assert(table[i]==before[i]);
    FM5DXGITestBackends(hwnd_call,core_call);
    for(pass=0;pass<3;++pass) {
        void *device=pass==0?&proxy:pass==1?&native_device:NULL;
        expected_device=pass==0?&native_device:device;
        expected_status=pass==2?E_INVALIDARG:S_FALSE;
        returned_object=pass==2?NULL:(void *)(ULONG_PTR)0x123456789abcdef0;
        expected_window=(HWND)(ULONG_PTR)0xabcdef0123456789;
        out=(void *)1;
        assert(((FM5CreateSwapChainForHwnd)table[16])(&factory,device,expected_window,
            &description,&fullscreen,&restricted,&out)==expected_status && out==returned_object);
        out=(void *)1;
        assert(((FM5CreateSwapChainForCoreWindow)table[17])(&factory,device,&core_window,
            &description,&restricted,&out)==expected_status && out==returned_object);
    }
    expected_device=&native_device;
    assert(((FM5CreateSwapChainForHwnd)table[16])(&factory,&proxy,expected_window,
        &description,&fullscreen,&restricted,NULL)==E_POINTER);
    assert(((FM5CreateSwapChainForCoreWindow)table[17])(&factory,&proxy,&core_window,
        &description,&restricted,NULL)==E_POINTER);
    assert(unwrap_calls==8 && native_calls==8);
    HeapFree(GetProcessHeap(),0,image);
    puts("PASS: guarded Factory2 slots16/17, 64 fingerprint mutations, all 7/6 arguments and full-width stack/output/HRESULT forwarding, proxy/native/NULL borrowed Device unwrap, no ownership operations");
    return 0;
}
