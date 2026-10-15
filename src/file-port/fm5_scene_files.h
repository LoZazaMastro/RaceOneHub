#pragma once
#include <windows.h>
__declspec(dllexport) void FM5SetSceneFileRate(UINT target);
__declspec(dllexport) HANDLE WINAPI FM5CreateGameFileW(LPCWSTR, DWORD, DWORD, LPSECURITY_ATTRIBUTES, DWORD, DWORD, HANDLE);
