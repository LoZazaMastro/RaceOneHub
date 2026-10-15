#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>

typedef HRESULT (WINAPI *FM5_XAUDIO_CREATE)(void **, UINT32, UINT32, void *);
__declspec(dllexport) HRESULT WINAPI FM5XAudio2Create(void **output, UINT32 flags,
    UINT32 processor, void *context);
#ifdef FM5_BRIDGE_TEST
void FM5AudioSetTestBackend(FM5_XAUDIO_CREATE backend);
#endif
