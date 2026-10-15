#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef HRESULT (WINAPI *FM5ActivationBackend)(const wchar_t *, const GUID *, void **);
HRESULT FM5ConfigureActivation(FM5ActivationBackend backend);
HRESULT WINAPI FM5GetActivationFactory(const wchar_t *name, const GUID *iid, void **out);
