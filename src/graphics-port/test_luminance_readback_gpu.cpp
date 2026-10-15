#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma warning(push)
#pragma warning(disable:4201)
#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <dxgi1_4.h>
#pragma warning(pop)
#include <wrl/client.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "../allocation-port/fm5_luminance_readback.h"
#include "fm5_queue_diagnostics.h"

using Microsoft::WRL::ComPtr;
typedef void (*TestBarriers)(void*,UINT,const D3D12_RESOURCE_BARRIER*);
typedef void (*TestCopy)(void*,const D3D12_TEXTURE_COPY_LOCATION*,UINT,UINT,UINT,const D3D12_TEXTURE_COPY_LOCATION*,const D3D12_BOX*);
extern "C" void FM5ReadbackTestConfigure(const void*,TestBarriers,TestCopy);
extern "C" BOOL FM5ReadbackTestArm(void*,void*,void*,void*);
extern "C" BOOL FM5ReadbackTestPoll(void);
extern "C" UINT FM5ReadbackTestPhase(void);
extern "C" BOOL FM5ReadbackTestResult(UINT*);

extern "C" void FM5DiagnosticWriteJsonLine(const char *text,DWORD length) {
    fwrite(text,1,length,stderr);
}

static UINT checks;
static void require(bool value,const char *description) {
    ++checks;
    if(!value){fprintf(stderr,"FAIL %s\n",description);ExitProcess(1);}
}
static void okay(HRESULT result,const char *description) {
    ++checks;
    if(FAILED(result)){fprintf(stderr,"FAIL %s HRESULT=%08lx\n",description,(unsigned long)result);ExitProcess(1);}
}
static D3D12_HEAP_PROPERTIES heap(D3D12_HEAP_TYPE type) {
    D3D12_HEAP_PROPERTIES p={};p.Type=type;p.CreationNodeMask=p.VisibleNodeMask=1;return p;
}
static D3D12_RESOURCE_DESC buffer_description(UINT64 bytes) {
    D3D12_RESOURCE_DESC d={};d.Dimension=D3D12_RESOURCE_DIMENSION_BUFFER;d.Width=bytes;
    d.Height=1;d.DepthOrArraySize=d.MipLevels=1;d.SampleDesc.Count=1;d.Layout=D3D12_TEXTURE_LAYOUT_ROW_MAJOR;return d;
}
static D3D12_RESOURCE_BARRIER transition(ID3D12Resource *resource,D3D12_RESOURCE_STATES before,D3D12_RESOURCE_STATES after) {
    D3D12_RESOURCE_BARRIER b={};b.Type=D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;b.Transition.pResource=resource;
    b.Transition.Subresource=0;b.Transition.StateBefore=before;b.Transition.StateAfter=after;return b;
}

int main(void) {
    SetEnvironmentVariableW(L"FM5_QUEUE_DIAGNOSTICS",L"0");
    ComPtr<ID3D12Debug> debug;
    const bool debug_available=SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debug)));
    if(debug_available)debug->EnableDebugLayer();
    ComPtr<IDXGIFactory4> factory;okay(CreateDXGIFactory1(IID_PPV_ARGS(&factory)),"CreateDXGIFactory1");
    ComPtr<IDXGIAdapter> adapter;okay(factory->EnumWarpAdapter(IID_PPV_ARGS(&adapter)),"WARP adapter");
    ComPtr<ID3D12Device> device;okay(D3D12CreateDevice(adapter.Get(),D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&device)),"Real WARP D3D12 device");
    ComPtr<ID3D12InfoQueue> info;
    if(debug_available)okay(device.As(&info),"Debug info queue");
    D3D12_COMMAND_QUEUE_DESC q={};q.Type=D3D12_COMMAND_LIST_TYPE_DIRECT;
    ComPtr<ID3D12CommandQueue> queue;okay(device->CreateCommandQueue(&q,IID_PPV_ARGS(&queue)),"Create queue");
    const UINT known_bits[]={0x3e000000,0x3f800000,0xc0000000}; // .125,1,-2
    UINT completed_cases=0;
    for(UINT expected:known_bits) {
        FM5ReadbackTestConfigure(NULL,NULL,NULL);
        ComPtr<ID3D12CommandAllocator> allocator;
        okay(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,IID_PPV_ARGS(&allocator)),"Create allocator");
        ComPtr<ID3D12GraphicsCommandList> list;
        okay(device->CreateCommandList(0,D3D12_COMMAND_LIST_TYPE_DIRECT,allocator.Get(),NULL,IID_PPV_ARGS(&list)),"Create list");
        D3D12_RESOURCE_DESC texture={};texture.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;texture.Width=texture.Height=1;
        texture.DepthOrArraySize=texture.MipLevels=1;texture.Format=DXGI_FORMAT_R32_FLOAT;texture.SampleDesc.Count=1;
        auto default_heap=heap(D3D12_HEAP_TYPE_DEFAULT);
        ComPtr<ID3D12Resource> physical;
        okay(device->CreateCommittedResource(&default_heap,D3D12_HEAP_FLAG_NONE,&texture,D3D12_RESOURCE_STATE_COPY_DEST,NULL,IID_PPV_ARGS(&physical)),"Create known R32_FLOAT texture");
        D3D12_PLACED_SUBRESOURCE_FOOTPRINT footprint={};UINT rows=0;UINT64 row_bytes=0,total=0;
        device->GetCopyableFootprints(&texture,0,1,0,&footprint,&rows,&row_bytes,&total);
        require(footprint.Offset==0 && footprint.Footprint.RowPitch==256 && rows==1 && row_bytes==4 && total<=256,"Exact native footprint");
        auto desc=buffer_description(256);auto upload_heap=heap(D3D12_HEAP_TYPE_UPLOAD);auto readback_heap=heap(D3D12_HEAP_TYPE_READBACK);
        ComPtr<ID3D12Resource> upload,readback;
        okay(device->CreateCommittedResource(&upload_heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_GENERIC_READ,NULL,IID_PPV_ARGS(&upload)),"Create upload");
        okay(device->CreateCommittedResource(&readback_heap,D3D12_HEAP_FLAG_NONE,&desc,D3D12_RESOURCE_STATE_COPY_DEST,NULL,IID_PPV_ARGS(&readback)),"Create readback");
        void *mapped=NULL;D3D12_RANGE no_read={0,0};
        okay(upload->Map(0,&no_read,&mapped),"Map upload");memset(mapped,0xa5,256);memcpy(mapped,&expected,4);
        D3D12_RANGE written={0,4};upload->Unmap(0,&written);
        D3D12_TEXTURE_COPY_LOCATION dst_texture={};dst_texture.pResource=physical.Get();dst_texture.Type=D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
        D3D12_TEXTURE_COPY_LOCATION src_upload={};src_upload.pResource=upload.Get();src_upload.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;src_upload.PlacedFootprint=footprint;
        list->CopyTextureRegion(&dst_texture,0,0,0,&src_upload,NULL);
        const auto shader_state=(D3D12_RESOURCE_STATES)(D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE|D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        auto b=transition(physical.Get(),D3D12_RESOURCE_STATE_COPY_DEST,shader_state);list->ResourceBarrier(1,&b);
        auto copy_state=(D3D12_RESOURCE_STATES)(shader_state|D3D12_RESOURCE_STATE_COPY_SOURCE);
        b=transition(physical.Get(),shader_state,copy_state);list->ResourceBarrier(1,&b);
        ComPtr<ID3D12Fence> fence;okay(device->CreateFence(0,D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(&fence)),"Create diagnostic private fence");
        require(fence->GetCompletedValue()==0,"Fence starts incomplete");
        require(FM5ReadbackTestArm(physical.Get(),readback.Get(),queue.Get(),fence.Get())!=FALSE,"Arm real resources");
        D3D12_TEXTURE_COPY_LOCATION dst_readback={};dst_readback.pResource=readback.Get();dst_readback.Type=D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;dst_readback.PlacedFootprint=footprint;
        FM5ReadbackCopyTextureRegion(list.Get(),&dst_readback,0,0,0,&dst_texture,NULL);
        require(FM5ReadbackTestPhase()==2,"Forwarder recorded actual COM list");
        UINT result=0;require(!FM5ReadbackTestResult(&result),"No result before submission");
        require(!FM5ReadbackTestPoll(),"Recorded job is not GPU-complete");
        b=transition(physical.Get(),copy_state,shader_state);list->ResourceBarrier(1,&b);
        okay(list->Close(),"Close real copy list");
        void *raw_lists[]={list.Get()};
        FM5DiagnosticExecuteCommandLists(queue.Get(),1,raw_lists,NULL,0);
        require(FM5ReadbackTestPhase()==3,"Genuine Execute then private fence Signal");
        const ULONGLONG deadline=GetTickCount64()+1800;
        while(!FM5ReadbackTestPoll() && GetTickCount64()<deadline)Sleep(1);
        require(FM5ReadbackTestPhase()==0,"Bounded real-fence completion");
        require(FM5ReadbackTestResult(&result)!=FALSE && result==expected,"Exact known float bits after genuine GPU copy");
        require(fence->GetCompletedValue()>=1,"Private fence completed before result");
        FM5ReadbackTestConfigure(NULL,NULL,NULL);++completed_cases;
    }
    UINT errors=0;
    if(info)for(UINT64 i=0;i<info->GetNumStoredMessages();++i){
        SIZE_T bytes=0;okay(info->GetMessage(i,NULL,&bytes),"Query debug message size");
        auto message=(D3D12_MESSAGE*)HeapAlloc(GetProcessHeap(),0,bytes);require(message!=NULL,"Debug message storage");
        okay(info->GetMessage(i,message,&bytes),"Read debug message");
        if(message->Severity==D3D12_MESSAGE_SEVERITY_ERROR||message->Severity==D3D12_MESSAGE_SEVERITY_CORRUPTION){
            ++errors;fprintf(stderr,"D3D12 ERROR %u %s\n",message->ID,message->pDescription);
        }
        HeapFree(GetProcessHeap(),0,message);
    }
    require(errors==0,"No observed D3D12 debug errors");
    printf("{\"passed\":true,\"adapter\":\"WARP\",\"real_d3d12\":true,\"cases\":%u,\"checks\":%u,\"debug_layer_available\":%s,\"debug_errors\":%u,\"expected_bits\":[\"3e000000\",\"3f800000\",\"c0000000\"],\"scope\":\"Actual module CopyTextureRegion forwarder, real Execute callback and diagnostic private fence. No game, GUI, user process input or native Az integration.\"}\n",
        completed_cases,checks,debug_available?"true":"false",errors);
    return 0;
}
