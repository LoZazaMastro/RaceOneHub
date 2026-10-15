#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define FM5_E_RESOURCE_METADATA ((DWORD)0xe05f0055)
HRESULT WINAPI FM5CreateLegacyResource(void *native_device, UINT slot, const void *description, const void *initial, void **out);
HRESULT WINAPI FM5CreateLegacyPlacementBuffer(void *native_device,const void *description,void *placement,void **out);
HRESULT WINAPI FM5CreateLegacyPlacementTexture2D(void *native_device,const void *description,UINT packing_a,UINT packing_b,void *placement,void **out);
HRESULT WINAPI FM5CreateLegacyPlacementTexture3D(void *native_device,const void *description,UINT packing_a,UINT packing_b,void *placement,void **out);
/* Borrowed, guarded swapchain Texture2D; records no extra native reference. */
HRESULT WINAPI FM5AdoptLegacyTexture2D(void *native_resource);
HRESULT WINAPI FM5MetadataGetDesc(void *native_resource, void *out, SIZE_T capacity);
HRESULT WINAPI FM5GetLegacyTexture2DDescription(void *native_resource,void *out48);
void WINAPI FM5LegacyGetDescription(void *native_resource, void *out);
void WINAPI FM5ForgetLegacyResource(void *native_resource);
#ifdef FM5_RESOURCE_TEST
void FM5ResourceFailAllocation(LONG countdown);
SIZE_T FM5ResourceMetadataCount(void);
#endif
