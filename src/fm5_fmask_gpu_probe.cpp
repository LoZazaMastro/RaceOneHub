#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
#include <cmath>
#include <cstdio>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>

using Microsoft::WRL::ComPtr;

static void check(HRESULT hr, const char *operation) {
    if (FAILED(hr)) {
        char message[256];
        sprintf_s(message, "%s: 0x%08lx", operation, static_cast<unsigned long>(hr));
        throw std::runtime_error(message);
    }
}

static ComPtr<ID3DBlob> compile(const std::string& source, const char *target) {
    ComPtr<ID3DBlob> code, errors;
    HRESULT hr = D3DCompile(source.data(), source.size(), "fm5-fmask-probe", nullptr,
        nullptr, "main", target, D3DCOMPILE_ENABLE_STRICTNESS, 0, &code, &errors);
    if (FAILED(hr) && errors) std::fputs(static_cast<const char *>(errors->GetBufferPointer()), stderr);
    check(hr, "D3DCompile");
    return code;
}

static std::string load_shader(const char *path) {
    std::ifstream file(path, std::ios::binary);
    if (!file) throw std::runtime_error("Cannot read extracted guest shader");
    std::string source((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    const std::string include = "#include \"ImageProcessor.sh\"";
    size_t where = source.find(include);
    if (where != 0) throw std::runtime_error("Unexpected guest shader prefix");
    source.replace(where, include.size(), "struct PS_INPUT { float2 Uv : TEXCOORD0; };\n");
    return source;
}

static void run_case(ID3D11Device *device, ID3D11DeviceContext *context,
                     ID3D11VertexShader *vertex, ID3D11PixelShader *sample_writer,
                     const char *shader_path, UINT fragments, DXGI_FORMAT mask_format,
                     UINT packed_mask) {
    UINT quality_levels = 0;
    check(device->CheckMultisampleQualityLevels(DXGI_FORMAT_R11G11B10_FLOAT,
        fragments, &quality_levels), "CheckMultisampleQualityLevels");
    if (!quality_levels) throw std::runtime_error("Required MSAA format unsupported on hardware");
    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = desc.Height = desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R11G11B10_FLOAT;
    desc.SampleDesc.Count = fragments;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    ComPtr<ID3D11Texture2D> colors, mask, output, staging;
    check(device->CreateTexture2D(&desc, nullptr, &colors), "Create MSAA color texture");
    ComPtr<ID3D11RenderTargetView> color_target, output_target;
    check(device->CreateRenderTargetView(colors.Get(), nullptr, &color_target), "Create MSAA RTV");
    ComPtr<ID3D11ShaderResourceView> color_view, mask_view;
    check(device->CreateShaderResourceView(colors.Get(), nullptr, &color_view), "Create MSAA SRV");
    desc.Format = mask_format;
    desc.SampleDesc.Count = 1;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.Usage = D3D11_USAGE_IMMUTABLE;
    D3D11_SUBRESOURCE_DATA initial = { &packed_mask, 4, 4 };
    check(device->CreateTexture2D(&desc, &initial, &mask), "Create expanded FMASK texture");
    check(device->CreateShaderResourceView(mask.Get(), nullptr, &mask_view), "Create FMASK SRV");
    desc.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET;
    desc.Usage = D3D11_USAGE_DEFAULT;
    check(device->CreateTexture2D(&desc, nullptr, &output), "Create resolve output");
    check(device->CreateRenderTargetView(output.Get(), nullptr, &output_target), "Create output RTV");
    desc.BindFlags = 0;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    check(device->CreateTexture2D(&desc, nullptr, &staging), "Create staging texture");
    auto code = compile(load_shader(shader_path), "ps_5_0");
    ComPtr<ID3D11PixelShader> resolve;
    check(device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(),
        nullptr, &resolve), "Create original FMASK resolve shader");
    ID3D11RenderTargetView *target = color_target.Get();
    context->OMSetRenderTargets(1, &target, nullptr);
    D3D11_VIEWPORT viewport = {0, 0, 1, 1, 0, 1};
    context->RSSetViewports(1, &viewport);
    context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    context->VSSetShader(vertex, nullptr, 0);
    context->PSSetShader(sample_writer, nullptr, 0);
    context->Draw(3, 0);
    target = output_target.Get();
    context->OMSetRenderTargets(1, &target, nullptr);
    ID3D11ShaderResourceView *inputs[] = { color_view.Get(), mask_view.Get() };
    context->PSSetShaderResources(0, 2, inputs);
    context->PSSetShader(resolve.Get(), nullptr, 0);
    context->Draw(3, 0);
    context->ClearState();
    context->CopyResource(staging.Get(), output.Get());
    D3D11_MAPPED_SUBRESOURCE mapped = {};
    check(context->Map(staging.Get(), 0, D3D11_MAP_READ, 0, &mapped), "Read GPU resolve");
    float result[4];
    memcpy(result, mapped.pData, sizeof(result));
    context->Unmap(staging.Get(), 0);
    const float expected[] = {0.5f, 0.5f, fragments == 2 ? 0.0f : 0.5f, 1.0f};
    for (UINT i = 0; i < 4; ++i)
        if (std::fabs(result[i] - expected[i]) > 0.0001f)
            throw std::runtime_error("Original shader produced an unexpected expanded FMASK resolve");
    std::printf("{\"fragments\":%u,\"expanded_mask\":%u,\"rgba\":[%.8f,%.8f,%.8f,%.8f]}\n",
        fragments, packed_mask, result[0], result[1], result[2], result[3]);
}

int main(int argc, char **argv) {
    if (argc != 3) return 2;
    try {
        ComPtr<ID3D11Device> device;
        ComPtr<ID3D11DeviceContext> context;
        D3D_FEATURE_LEVEL selected;
        const D3D_FEATURE_LEVEL requested[] = {D3D_FEATURE_LEVEL_11_1};
        check(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, requested,
            1, D3D11_SDK_VERSION, &device, &selected, &context), "Create hardware device");
        const char *vs = "struct O {float4 p:SV_POSITION;float2 uv:TEXCOORD0;};"
            "O main(uint id:SV_VertexID){O o;o.p=float4(id==2?3:-1,id==1?3:-1,0,1);o.uv=0;return o;}";
        const char *ps = "float4 main(uint i:SV_SampleIndex):SV_TARGET{"
            "float4 c[4]={float4(1,0,0,1),float4(0,1,0,1),float4(0,0,1,1),float4(1,1,1,1)};return c[i];}";
        auto vc = compile(vs, "vs_5_0"), pc = compile(ps, "ps_5_0");
        ComPtr<ID3D11VertexShader> vertex;
        ComPtr<ID3D11PixelShader> sample_writer;
        check(device->CreateVertexShader(vc->GetBufferPointer(), vc->GetBufferSize(),
            nullptr, &vertex), "Create probe vertex shader");
        check(device->CreatePixelShader(pc->GetBufferPointer(), pc->GetBufferSize(),
            nullptr, &sample_writer), "Create sample writer");
        D3D11_RASTERIZER_DESC raster = {};
        raster.FillMode = D3D11_FILL_SOLID;
        raster.CullMode = D3D11_CULL_NONE;
        raster.DepthClipEnable = raster.MultisampleEnable = TRUE;
        ComPtr<ID3D11RasterizerState> state;
        check(device->CreateRasterizerState(&raster, &state), "Create rasterizer");
        context->RSSetState(state.Get());
        run_case(device.Get(), context.Get(), vertex.Get(), sample_writer.Get(),
            argv[1], 2, DXGI_FORMAT_R8_UINT, 0xa4);
        context->RSSetState(state.Get());
        run_case(device.Get(), context.Get(), vertex.Get(), sample_writer.Get(),
            argv[2], 4, DXGI_FORMAT_R32_UINT, 0x44443210);
        context->ClearState();
        context->Flush();
        return 0;
    } catch (const std::exception& error) {
        std::fprintf(stderr, "%s\n", error.what());
        return 1;
    }
}
