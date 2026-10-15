#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <stddef.h>

/* The description/output types stay opaque: every bit is forwarded unchanged.
 * Both prototypes follow the verified x64 native DXGI factory methods. */
typedef HRESULT (WINAPI *FM5CreateSwapChainForHwnd)(void *, void *, HWND,
    const void *, const void *, void *, void **);
typedef HRESULT (WINAPI *FM5CreateSwapChainForCoreWindow)(void *, void *, void *,
    const void *, void *, void **);

typedef HRESULT (WINAPI *FM5GetSwapChainBuffer)(void *,UINT,const void *,void **);

HRESULT FM5PatchFactoryVtable(void **table, UINT kind,
    const unsigned char *base, size_t size);

#ifdef FM5_DXGI_TEST
void FM5DXGITestGetBuffer(FM5GetSwapChainBuffer backend);
void FM5DXGITestBackends(FM5CreateSwapChainForHwnd hwnd_backend,
    FM5CreateSwapChainForCoreWindow core_backend);
#endif
