#pragma once

typedef long FM5HRESULT;
typedef unsigned int FM5UINT;
typedef unsigned long long FM5UINT64;
typedef struct FM5Guid {
    unsigned int a;
    unsigned short b, c;
    unsigned char d[8];
} FM5Guid;

typedef struct FM5CreateDescriptor {
    FM5UINT version;
    FM5UINT flags;
    FM5UINT64 reserved0, reserved1;
    FM5UINT hardware_mask;
    FM5UINT reserved2, reserved3, reserved4, reserved5;
} FM5CreateDescriptor;

typedef FM5HRESULT (*FM5CreateDeviceX)(const FM5CreateDescriptor *, void **, void **);

#ifdef FM5_GRAPHICS_EXPORTS
#define FM5_API __declspec(dllexport)
#else
#define FM5_API
#endif

/* Normalization is opt-in until the two SDKs' flag semantics are verified. */
#define FM5_GRAPHICS_ALLOW_DESCRIPTOR_NORMALIZATION 1u
/* Production integration compiles fm5_graphics_bridge.c with
 * FM5_GRAPHICS_PRODUCTION_INTEROP. It adopts the native factory QI outputs
 * through the shared Device/Context proxies and configures root policies.
 * The standalone factory ABI harness deliberately leaves this undefined. */

FM5_API FM5HRESULT FM5ConfigureGraphicsBridge(FM5CreateDeviceX factory, FM5UINT policy);
FM5_API FM5HRESULT ForzaMastro5D3D11CreateDevice(
    void *adapter, FM5UINT driver_type, void *software, FM5UINT flags,
    const FM5UINT *feature_levels, FM5UINT feature_level_count,
    FM5UINT sdk_version, void **device, FM5UINT *selected_level, void **context);
FM5_API FM5HRESULT FM5D3D11CreateDevice(
    void *adapter, FM5UINT driver_type, void *software, FM5UINT flags,
    const FM5UINT *feature_levels, FM5UINT feature_level_count,
    FM5UINT sdk_version, void **device, FM5UINT *selected_level, void **context);
