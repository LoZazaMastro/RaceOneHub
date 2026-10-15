#define WIN32_LEAN_AND_MEAN
#include "fm5_pso_observer.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

static const D3D12_GRAPHICS_PIPELINE_STATE_DESC *expected_description;
static const IID *expected_iid;
static void **expected_output;
static void *expected_device;
static HRESULT result;
static unsigned calls, queries;
static HRESULT WINAPI create(void *device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *description,
                              REFIID iid, void **output) {
    assert(device == expected_device && description == expected_description);
    assert(&iid == expected_iid && output == expected_output);
    ++calls;
    *output = (void *)(uintptr_t)0x123456789abcdef0;
    return result;
}
static HRESULT WINAPI check(void *device, D3D12_FEATURE feature, void *data, UINT size) {
    assert(device == expected_device && feature == D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS);
    assert(size == sizeof(D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS));
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS *levels =
        (D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS *)data;
    assert(levels->SampleCount == 4 && levels->Flags == D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE);
    assert(levels->Format == DXGI_FORMAT_R8G8B8A8_UNORM || levels->Format == DXGI_FORMAT_D32_FLOAT);
    levels->NumQualityLevels = 1;
    ++queries;
    return S_OK;
}
int main() {
    void *vtable[44] = {};
    vtable[10] = (void *)&create;
    vtable[13] = (void *)&check;
    void **object = vtable;
    expected_device = &object;
    D3D12_GRAPHICS_PIPELINE_STATE_DESC description = {};
    description.SampleDesc.Count = 4;
    description.SampleDesc.Quality = 1;
    description.NumRenderTargets = 1;
    description.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    description.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    unsigned char blob[7] = {};
    description.CachedPSO = {blob, sizeof(blob)};
    D3D12_GRAPHICS_PIPELINE_STATE_DESC before = description;
    IID iid = {0x765a30f3, 0xf624, 0x4c6f, {0xa8, 0x28, 0xac, 0xe9, 0x48, 0x62, 0x24, 0x45}};
    void *output = NULL;
    expected_description = &description;
    expected_iid = &iid;
    expected_output = &output;
    SetEnvironmentVariableA("FM5_D3D12_DEBUG", NULL);
    for (unsigned i = 0; i < 3; ++i) {
        result = i == 0 ? S_OK : E_INVALIDARG;
        if (i == 2) SetEnvironmentVariableA("FM5_D3D12_DEBUG", "1");
        assert(FM5CreateGraphicsPsoObserved((ID3D12Device *)&object, &description, iid, &output) == result);
        assert(output == (void *)(uintptr_t)0x123456789abcdef0);
        assert(memcmp(&description, &before, sizeof(before)) == 0);
    }
    assert(calls == 3 && queries == 2);
    assert(FM5CreateGraphicsPsoObserved((ID3D12Device *)&object, &description, iid, &output) == E_INVALIDARG);
    assert(calls == 4 && queries == 2);
    puts("PASS: exact device/descriptor/IID/output/HRESULT forwarding; no descriptor mutation; debug-gated failure observation; actual format/count capability deduplication");
    return 0;
}
