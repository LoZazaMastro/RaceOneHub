#define FM5_GRAPHICS_EXPORTS
#include "fm5_graphics_bridge.h"
#include "factory_signature.h"

#ifdef FM5_GRAPHICS_PRODUCTION_INTEROP
#include "fm5_performance_context.h"
#include "fm5_scissor_state.h"
HRESULT WINAPI FM5WrapPerformanceDevice(void *native_owned, void **result);
HRESULT FM5ApplyNativeCompletionPolicy(void *native_context, BOOL enabled);
HRESULT FM5ApplyNativeTopology(void *native_context, UINT topology);
HRESULT WINAPI FM5GetLegacyTexture2DDescription(void *resource, void *description48);
#ifndef FM5_BRIDGE_TEST
#include "fm5_color_observer.h"
#include "../allocation-port/fm5_msaa4.h"
void FM5RunNativeScissorProbe(void *wrapped_device,void *wrapped_context);
#endif
#endif

#define FM5_E_INVALIDARG ((FM5HRESULT)0x80070057L)
#define FM5_E_NOTIMPL ((FM5HRESULT)0x80004001L)
#define FM5_E_UNEXPECTED ((FM5HRESULT)0x8000FFFFL)
#define FM5_E_POINTER ((FM5HRESULT)0x80004003L)

typedef FM5HRESULT (*QueryInterfaceFn)(void *, const FM5Guid *, void **);
typedef unsigned long (*ReleaseFn)(void *);

static FM5CreateDeviceX configured_factory;
static FM5UINT configured_policy;
static const FM5Guid device_iid = {
    0xdb6f6ddb, 0xac77, 0x4e88, {0x82,0x53,0x81,0x9d,0xf9,0xbb,0xf1,0x40}
};
static const FM5Guid context_iid = {
    0xc0bfa96c, 0xe089, 0x44fb, {0x8e,0xaf,0x26,0xf8,0x79,0x61,0x90,0xda}
};

static void release(void *object) {
    if (object) {
        void **vtable = *(void ***)object;
        ((ReleaseFn)vtable[2])(object);
    }
}

static FM5HRESULT query(void *object, const FM5Guid *iid, void **result) {
    void **vtable = *(void ***)object;
    return ((QueryInterfaceFn)vtable[0])(object, iid, result);
}

FM5HRESULT FM5ConfigureGraphicsBridge(FM5CreateDeviceX factory, FM5UINT policy) {
    FM5UINT index;
    if (policy & ~FM5_GRAPHICS_ALLOW_DESCRIPTOR_NORMALIZATION)
        return FM5_E_INVALIDARG;
#ifndef FM5_BRIDGE_TEST
    if (factory) {
        const unsigned char *bytes = (const unsigned char *)factory;
        for (index = 0; index != sizeof(factory_signature); ++index)
            if (bytes[index] != factory_signature[index])
                return FM5_E_INVALIDARG;
    }
#else
    (void)index;
#endif
    configured_factory = factory;
    configured_policy = policy;
#ifdef FM5_GRAPHICS_PRODUCTION_INTEROP
    FM5ConfigurePerformanceContext(FM5ApplyNativeCompletionPolicy);
    FM5ConfigureContextInterop(FM5ApplyNativeTopology, FM5WrapPerformanceDevice);
    FM5ConfigureContextResolve(FM5GetLegacyTexture2DDescription);
    FM5ConfigureContextScissors(FM5ReadNativeScissorSnapshot);
    FM5ConfigureContextSamplers(FM5GetNativePixelSamplers);
#ifndef FM5_BRIDGE_TEST
    {
        wchar_t diagnostic[2];
        BOOL enabled = GetEnvironmentVariableW(L"FM5_COLOR_DIAGNOSTICS", diagnostic, 2) == 1 && diagnostic[0] == L'1';
        FM5ConfigurePixelShaderObserver(enabled ? FM5ObservePixelShaderCreation : NULL);
        FM5ConfigureContextDrawObserver(enabled ? FM5ObservePostprocessDraw : NULL);
        FM5ConfigureContextMapObserver(enabled ? FM5ObservePostprocessMap : NULL,
                                      enabled ? FM5ObservePostprocessUnmap : NULL);
    }
#endif
#endif
    return 0;
}

FM5HRESULT ForzaMastro5D3D11CreateDevice(
    void *adapter, FM5UINT driver_type, void *software, FM5UINT flags,
    const FM5UINT *feature_levels, FM5UINT feature_level_count,
    FM5UINT sdk_version, void **device, FM5UINT *selected_level, void **context) {
    FM5CreateDescriptor descriptor;
    FM5HRESULT hr;
    void *device_x = 0, *context_x = 0, *base_device = 0, *base_context = 0;
    if (device) *device = 0;
    if (context) *context = 0;
    if (selected_level) *selected_level = 0;
    if (!device || !context) return FM5_E_POINTER;
    if (adapter || software || sdk_version != 0x10002 ||
        !feature_levels || feature_level_count != 1 || feature_levels[0] != 0xb100)
        return FM5_E_INVALIDARG;
    if (driver_type != 1 || (flags != 0x20 && flags != 0x08000020))
        return FM5_E_NOTIMPL;
    if (!configured_factory ||
        !(configured_policy & FM5_GRAPHICS_ALLOW_DESCRIPTOR_NORMALIZATION))
        return FM5_E_NOTIMPL;

    descriptor.version = 0x1000b;
    descriptor.flags = 0x04000000;
    descriptor.reserved0 = descriptor.reserved1 = 0;
    descriptor.hardware_mask = 0x3f;
    descriptor.reserved2 = descriptor.reserved3 = 0;
    descriptor.reserved4 = descriptor.reserved5 = 0;
    hr = configured_factory(&descriptor, &device_x, &context_x);
    if (hr < 0) goto done;
    if (!device_x || !context_x) {
        hr = FM5_E_UNEXPECTED;
        goto done;
    }
    hr = query(device_x, &device_iid, &base_device);
    if (hr < 0) goto done;
    hr = query(context_x, &context_iid, &base_context);
    if (hr < 0) goto done;
    if (!base_device || !base_context) {
        hr = FM5_E_UNEXPECTED;
        goto done;
    }
#ifdef FM5_GRAPHICS_PRODUCTION_INTEROP
    {
        void *wrapped = 0;
        hr = FM5WrapPerformanceDevice(base_device, &wrapped);
        if (hr < 0) goto done;
        base_device = wrapped;
        wrapped = 0;
        hr = FM5WrapPerformanceContext(base_context, &wrapped);
        if (hr < 0) goto done;
        base_context = wrapped;
    }
#endif
#if defined(FM5_GRAPHICS_PRODUCTION_INTEROP) && !defined(FM5_BRIDGE_TEST)
    FM5RunNativeScissorProbe(base_device,base_context);
#endif
    *device = base_device;
    *context = base_context;
    base_device = base_context = 0;
    if (selected_level) *selected_level = 0xb100;
done:
    release(base_context);
    release(base_device);
    release(context_x);
    release(device_x);
    return hr;
}

FM5HRESULT FM5D3D11CreateDevice(
    void *adapter, FM5UINT driver_type, void *software, FM5UINT flags,
    const FM5UINT *feature_levels, FM5UINT feature_level_count,
    FM5UINT sdk_version, void **device, FM5UINT *selected_level, void **context) {
    return ForzaMastro5D3D11CreateDevice(adapter, driver_type, software, flags,
        feature_levels, feature_level_count, sdk_version, device, selected_level, context);
}
