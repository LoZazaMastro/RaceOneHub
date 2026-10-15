#ifndef FM5_PSO_OBSERVER_H
#define FM5_PSO_OBSERVER_H
#include <windows.h>
#include <d3d12.h>
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) HRESULT WINAPI FM5CreateGraphicsPsoObserved(
    ID3D12Device *device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *description,
    REFIID iid, void **output);
#ifdef __cplusplus
}
#endif
#endif
