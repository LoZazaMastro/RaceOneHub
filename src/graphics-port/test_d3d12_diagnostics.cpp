#include "fm5_d3d12_diagnostics.h"
#pragma warning(push)
#pragma warning(disable: 4201)
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#pragma warning(pop)
#include <assert.h>
#include <stdio.h>

int main(void) {
    WCHAR value[2] = {0};
    BOOL enabled = GetEnvironmentVariableW(L"FM5_D3D12_DEBUG", value, 2) == 1 && value[0] == L'1';
    IDXGIFactory4 *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    ID3D12Device *device = NULL;
    ID3D12CommandAllocator *allocator = NULL;
    ID3D12GraphicsCommandList *list = NULL;
    ID3D12Resource *source = NULL, *destination = NULL;
    assert(SUCCEEDED(CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), (void **)&factory)));
    assert(SUCCEEDED(factory->EnumWarpAdapter(__uuidof(IDXGIAdapter), (void **)&adapter)));
    assert(SUCCEEDED(FM5DiagnosticD3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0,
        &__uuidof(ID3D12Device), (void **)&device)) && device);
    ID3D12InfoQueue1 *queue = NULL;
    HRESULT queue_hr = device->QueryInterface(__uuidof(ID3D12InfoQueue1), (void **)&queue);
    assert(enabled ? SUCCEEDED(queue_hr) : FAILED(queue_hr));
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
        __uuidof(ID3D12CommandAllocator), (void **)&allocator)));
    assert(SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        allocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&list)));
    if (enabled) {
        D3D12_HEAP_PROPERTIES heap = {};
        D3D12_RESOURCE_DESC description = {};
        heap.Type = D3D12_HEAP_TYPE_DEFAULT;
        description.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        description.Width = 1024; description.Height = 1;
        description.DepthOrArraySize = description.MipLevels = 1;
        description.SampleDesc.Count = 1; description.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        assert(SUCCEEDED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE,
            &description, D3D12_RESOURCE_STATE_COPY_SOURCE, NULL,
            __uuidof(ID3D12Resource), (void **)&source)));
        assert(SUCCEEDED(device->CreateCommittedResource(&heap, D3D12_HEAP_FLAG_NONE,
            &description, D3D12_RESOURCE_STATE_COPY_DEST, NULL,
            __uuidof(ID3D12Resource), (void **)&destination)));
        /* Genuine out-of-bounds recording error. Never submit this list. */
        list->CopyBufferRegion(destination, 0, source, 0, 2048);
        assert(queue->GetNumStoredMessages() > 0);
        /* Also proves the callback JSON encoder preserves quotes/newlines. */
        assert(SUCCEEDED(queue->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_ERROR,
            "fixture \"quoted\"\nnext line\\path")));
        assert(SUCCEEDED(queue->AddApplicationMessage(D3D12_MESSAGE_SEVERITY_WARNING,
            "fixture warning intentionally excluded")));
    }
    HRESULT close_hr = list->Close();
    assert(enabled ? FAILED(close_hr) : SUCCEEDED(close_hr));
    printf("PASS diagnostics mode=%s queue=0x%08lx close=0x%08lx noGPUsubmission\n",
        enabled ? "enabled" : "disabled", (unsigned long)queue_hr, (unsigned long)close_hr);
    if (queue) queue->Release();
    list->Release(); allocator->Release();
    if (source) source->Release();
    if (destination) destination->Release();
    device->Release(); adapter->Release(); factory->Release();
    return 0;
}
