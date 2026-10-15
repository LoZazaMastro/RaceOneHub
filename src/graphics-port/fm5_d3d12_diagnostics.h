#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Typed x64 D3D12CreateDevice boundary. Env FM5_D3D12_DEBUG=1 enables
 * process-local CPU validation before the first device; GPU validation stays off. */
__declspec(dllexport) HRESULT WINAPI FM5DiagnosticD3D12CreateDevice(
    void *adapter, UINT minimum_feature_level, const GUID *iid, void **out);
/* Internal shared sink serializes complete observer records with InfoQueue. */
void FM5DiagnosticWriteJsonLine(const char *text, DWORD length);
#ifdef __cplusplus
}
#endif
