#define WIN32_LEAN_AND_MEAN
#include "fm5_pso_observer.h"
#include <stdio.h>
#include <stdint.h>
#include <string.h>

/* Call-site observer only. The original D3D12 descriptor, IID, output pointer
   and HRESULT cross this boundary unchanged. No device vtable is modified. */
namespace {
struct CapabilityKey { UINT format, samples; };
SRWLOCK capabilities_lock = SRWLOCK_INIT;
CapabilityKey capabilities[64];
UINT capability_count;
volatile LONG failure_count;
SRWLOCK shaders_lock = SRWLOCK_INIT;
uint64_t observed_shaders[8];
UINT observed_shader_count;

void emit(const char *line, int length) {
    DWORD written;
    if (length > 0) WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, (DWORD)length, &written, NULL);
}

void inspect_resolve_shader(const D3D12_SHADER_BYTECODE &shader) {
    if (!shader.pShaderBytecode || shader.BytecodeLength < 32 || shader.BytecodeLength > 1048576) return;
    const BYTE *bytes = (const BYTE *)shader.pShaderBytecode;
    if (memcmp(bytes, "DXBC", 4)) return;
    UINT total, count;
    memcpy(&total, bytes + 24, 4); memcpy(&count, bytes + 28, 4);
    if (total != shader.BytecodeLength || count > 32 || 32 + count * 4 > total) return;
    const BYTE *code = NULL;
    UINT code_size = 0;
    bool color = false, mask = false;
    for (UINT i = 0; i < count; ++i) {
        UINT offset, length;
        memcpy(&offset, bytes + 32 + i * 4, 4);
        if (offset > total - 8) return;
        memcpy(&length, bytes + offset + 4, 4);
        if (length > total - offset - 8) return;
        if (!memcmp(bytes + offset, "SHEX", 4) || !memcmp(bytes + offset, "SHDR", 4)) {
            code = bytes + offset + 8; code_size = length;
        }
        if (!memcmp(bytes + offset, "RDEF", 4)) {
            const BYTE *data = bytes + offset + 8;
            for (UINT n = 0; n < length; ++n) {
                if (length - n >= 13 && !memcmp(data + n, "TexColorMSAA", 13)) color = true;
                if (length - n >= 9 && !memcmp(data + n, "TexFMask", 9)) mask = true;
            }
        }
    }
    if (!color || !mask || !code || code_size > 4096) return;
    uint64_t hash = UINT64_C(14695981039346656037);
    for (UINT i = 0; i < code_size; ++i) hash = (hash ^ code[i]) * UINT64_C(1099511628211);
    bool record = false;
    AcquireSRWLockExclusive(&shaders_lock);
    UINT i;
    for (i = 0; i < observed_shader_count; ++i) if (observed_shaders[i] == hash) break;
    if (i == observed_shader_count && observed_shader_count < _countof(observed_shaders)) {
        observed_shaders[observed_shader_count++] = hash; record = true;
    }
    ReleaseSRWLockExclusive(&shaders_lock);
    if (!record) return;
    char line[8704];
    int used = _snprintf_s(line, sizeof(line), _TRUNCATE,
        "{\"event\":\"fm5_eqaa_shader_tokens\",\"blob_bytes\":%llu,\"code_bytes\":%u,\"code_fnv64\":\"%016llx\",\"code_hex\":\"",
        (unsigned long long)shader.BytecodeLength, code_size, (unsigned long long)hash);
    if (used < 0) return;
    static const char hex[] = "0123456789abcdef";
    for (i = 0; i < code_size; ++i) {
        line[used++] = hex[code[i] >> 4]; line[used++] = hex[code[i] & 15];
    }
    memcpy(line + used, "\"}\r\n", 4); used += 4;
    emit(line, used);
}

void inspect_capability(ID3D12Device *device, DXGI_FORMAT format, UINT samples) {
    if (format == DXGI_FORMAT_UNKNOWN) return;
    bool inspect = false;
    AcquireSRWLockExclusive(&capabilities_lock);
    UINT i;
    for (i = 0; i < capability_count; ++i)
        if (capabilities[i].format == (UINT)format && capabilities[i].samples == samples) break;
    if (i == capability_count && capability_count < _countof(capabilities)) {
        capabilities[capability_count++] = {(UINT)format, samples};
        inspect = true;
    }
    ReleaseSRWLockExclusive(&capabilities_lock);
    if (!inspect) return;
    D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS levels = {};
    levels.Format = format;
    levels.SampleCount = samples;
    levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
    HRESULT status = device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS,
                                               &levels, sizeof(levels));
    char line[384];
    int length = _snprintf_s(line, sizeof(line), _TRUNCATE,
        "{\"event\":\"fm5_pso_msaa_capability\",\"format\":%u,\"samples\":%u,"
        "\"quality_levels\":%u,\"hr\":\"0x%08lx\",\"thread\":%lu}\r\n",
        (UINT)format, samples, levels.NumQualityLevels, (unsigned long)status, GetCurrentThreadId());
    emit(line, length);
}
}

extern "C" __declspec(dllexport) HRESULT WINAPI FM5CreateGraphicsPsoObserved(
    ID3D12Device *device, const D3D12_GRAPHICS_PIPELINE_STATE_DESC *description,
    REFIID iid, void **output) {
    char enabled[4];
    bool debug = GetEnvironmentVariableA("FM5_D3D12_DEBUG", enabled, sizeof(enabled)) != 0;
    if (debug && description) inspect_resolve_shader(description->PS);
    HRESULT status = device->CreateGraphicsPipelineState(description, iid, output);
    if (FAILED(status)) {
        if (!debug) return status;
        LONG ordinal = InterlockedIncrement(&failure_count);
        if (description) {
            if (ordinal <= 32) {
                char line[1024];
                int length = _snprintf_s(line, sizeof(line), _TRUNCATE,
                    "{\"event\":\"fm5_pso_creation_failure\",\"ordinal\":%ld,"
                    "\"hr\":\"0x%08lx\",\"samples\":%u,\"quality\":%u,\"rtv_count\":%u,"
                    "\"rtv_formats\":[%u,%u,%u,%u,%u,%u,%u,%u],\"dsv_format\":%u,"
                    "\"cached_blob_bytes\":%llu,\"thread\":%lu}\r\n",
                    ordinal, (unsigned long)status, description->SampleDesc.Count,
                    description->SampleDesc.Quality, description->NumRenderTargets,
                    (UINT)description->RTVFormats[0], (UINT)description->RTVFormats[1],
                    (UINT)description->RTVFormats[2], (UINT)description->RTVFormats[3],
                    (UINT)description->RTVFormats[4], (UINT)description->RTVFormats[5],
                    (UINT)description->RTVFormats[6], (UINT)description->RTVFormats[7],
                    (UINT)description->DSVFormat,
                    (unsigned long long)description->CachedPSO.CachedBlobSizeInBytes,
                    GetCurrentThreadId());
                emit(line, length);
            }
            /* Query once per actual format/count pair, after the original
               creation failure and outside InfoQueue callbacks/our lock. */
            for (UINT i = 0; i < description->NumRenderTargets && i < 8; ++i)
                inspect_capability(device, description->RTVFormats[i], description->SampleDesc.Count);
            inspect_capability(device, description->DSVFormat, description->SampleDesc.Count);
        }
    }
    return status;
}
