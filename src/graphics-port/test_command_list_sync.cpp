#include "fm5_d3d12_diagnostics.h"
#include "fm5_queue_diagnostics.h"
#pragma warning(push)
#pragma warning(disable: 4201)
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_6.h>
#pragma warning(pop)
#include <assert.h>
#include <stdio.h>
#include <string.h>

int main(int argc, char **argv) {
    assert(argc == 2);
    BOOL duplicate = strcmp(argv[1], "duplicate") == 0;
    assert(duplicate || strcmp(argv[1], "reuse") == 0);
    SetEnvironmentVariableW(L"FM5_D3D12_DEBUG", L"1");
    SetEnvironmentVariableW(L"FM5_QUEUE_DIAGNOSTICS", L"1");
    IDXGIFactory4 *factory = NULL;
    IDXGIAdapter *adapter = NULL;
    ID3D12Device *device = NULL;
    ID3D12InfoQueue1 *info = NULL;
    ID3D12CommandQueue *queue = NULL;
    ID3D12Fence *gate = NULL;
    ID3D12CommandAllocator *allocator = NULL;
    ID3D12GraphicsCommandList *list = NULL;
    assert(SUCCEEDED(CreateDXGIFactory2(0, __uuidof(IDXGIFactory4), (void **)&factory)));
    assert(SUCCEEDED(factory->EnumWarpAdapter(__uuidof(IDXGIAdapter), (void **)&adapter)));
    assert(SUCCEEDED(FM5DiagnosticD3D12CreateDevice(adapter, D3D_FEATURE_LEVEL_11_0,
        &__uuidof(ID3D12Device), (void **)&device)));
    assert(SUCCEEDED(device->QueryInterface(__uuidof(ID3D12InfoQueue1), (void **)&info)));
    D3D12_COMMAND_QUEUE_DESC descriptor = {};
    descriptor.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    assert(SUCCEEDED(device->CreateCommandQueue(&descriptor,
        __uuidof(ID3D12CommandQueue), (void **)&queue)));
    assert(SUCCEEDED(device->CreateFence(0, D3D12_FENCE_FLAG_NONE,
        __uuidof(ID3D12Fence), (void **)&gate)));
    assert(SUCCEEDED(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
        __uuidof(ID3D12CommandAllocator), (void **)&allocator)));
    assert(SUCCEEDED(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT,
        allocator, NULL, __uuidof(ID3D12GraphicsCommandList), (void **)&list)));
    assert(SUCCEEDED(list->Close()));
    /* A real empty command list, no draw/resource access. The queue is gated
     * so previous execution cannot finish before validation of the second use. */
    assert(SUCCEEDED(queue->Wait(gate, 1)));
    ID3D12CommandList *batch[2] = { list, list };
    BYTE owner[32] = {};
    *(void **)(owner + 0x10) = queue;
    *(void **)(owner + 0x18) = gate;
    if (duplicate) FM5DiagnosticExecuteCommandLists(queue, 2, (void *const *)batch, owner, 1);
    else {
        FM5DiagnosticExecuteCommandLists(queue, 1, (void *const *)batch, owner, 1);
        FM5DiagnosticExecuteCommandLists(queue, 1, (void *const *)batch, owner, 2);
    }
    /* Always release the CPU-controlled gate before inspecting/releasing. */
    assert(SUCCEEDED(gate->Signal(1)));
    UINT sync_messages = 0;
    UINT64 inspected = 0;
    for (UINT attempt = 0; attempt < 100 && !sync_messages; ++attempt) {
      UINT64 count = info->GetNumStoredMessages();
      for (UINT64 i = inspected; i < count; ++i) {
        SIZE_T bytes = 0;
        assert(SUCCEEDED(info->GetMessage(i, NULL, &bytes)));
        D3D12_MESSAGE *message = (D3D12_MESSAGE *)HeapAlloc(GetProcessHeap(), 0, bytes);
        assert(message);
        assert(SUCCEEDED(info->GetMessage(i, message, &bytes)));
        if (message->ID == D3D12_MESSAGE_ID_COMMAND_LIST_SYNC) ++sync_messages;
        HeapFree(GetProcessHeap(), 0, message);
      }
      inspected = count;
      if (!sync_messages) Sleep(10);
    }
    assert(sync_messages > 0);
    /* Exercise the logger's eight-stack bound explicitly; these application
     * messages are labelled synthetic and are not evidence of another GPU fault. */
    for (UINT i = 0; i < 10; ++i)
        assert(SUCCEEDED(info->AddMessage(D3D12_MESSAGE_CATEGORY_EXECUTION,
            D3D12_MESSAGE_SEVERITY_ERROR, D3D12_MESSAGE_ID_COMMAND_LIST_SYNC,
            "synthetic stack-limit fixture")));
    printf("PASS genuine COMMAND_LIST_SYNC mode=%s native_messages=%u removed=0x%08lx noDraw\n",
        argv[1], sync_messages, (unsigned long)device->GetDeviceRemovedReason());
    list->Release(); allocator->Release(); gate->Release(); queue->Release();
    info->Release(); device->Release(); adapter->Release(); factory->Release();
    return 0;
}
