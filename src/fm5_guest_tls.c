#include "fm5_guest_tls.h"
#include <limits.h>
#include <string.h>

#define THUNK_BYTES 32u

static unsigned char *allocate_near(unsigned char *image, size_t image_size,
                                    size_t bytes) {
    SYSTEM_INFO info;
    uintptr_t begin, end, at, granularity;
    GetSystemInfo(&info);
    granularity = info.dwAllocationGranularity;
    begin = ((uintptr_t)image + image_size + granularity - 1) & ~(granularity - 1);
    end = (uintptr_t)image + 0x70000000u;
    for (at = begin; at < end;) {
        MEMORY_BASIC_INFORMATION region;
        uintptr_t next;
        if (!VirtualQuery((void *)at, &region, sizeof(region))) break;
        next = (uintptr_t)region.BaseAddress + region.RegionSize;
        if (region.State == MEM_FREE && next - at >= bytes) {
            unsigned char *result = VirtualAlloc((void *)at, bytes,
                MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
            if (result) return result;
        }
        if (next <= at) break;
        at = (next + granularity - 1) & ~(granularity - 1);
    }
    return NULL;
}

HRESULT FM5RedirectGuestTLS(unsigned char *image, size_t image_size,
    DWORD tls_index, const FM5TLSRedirectSite *sites, size_t count,
    FM5TLSRedirect *redirect) {
    unsigned char *code;
    size_t i, j, bytes;
    DWORD old_protect;
    if (!image || !sites || !redirect) return E_POINTER;
    memset(redirect, 0, sizeof(*redirect));
    if (!count || count > 4096 || !tls_index || tls_index > 65535)
        return E_INVALIDARG;
    for (i = 0; i < count; ++i) {
        unsigned char expected[12] = {0x65,0x48,0x8b,0x04,0x25,0x58,0,0,0};
        if (sites[i].destination != 0 && sites[i].destination != 1)
            return E_INVALIDARG;
        expected[3] |= (unsigned char)(sites[i].destination << 3);
        if (sites[i].patch_bytes == 12) {
            if (sites[i].destination != 0 ||
                (sites[i].lookup_destination != 1 && sites[i].lookup_destination != 2))
                return E_INVALIDARG;
            expected[9] = 0x48;
            expected[10] = 0x8b;
            expected[11] = (unsigned char)(sites[i].lookup_destination << 3);
        } else if (sites[i].patch_bytes != 9 || sites[i].lookup_destination != 0xff) {
            return E_INVALIDARG;
        }
        if (sites[i].rva > image_size || image_size - sites[i].rva < sites[i].patch_bytes ||
            memcmp(sites[i].expected, expected, sites[i].patch_bytes) ||
            memcmp(image + sites[i].rva, expected, sites[i].patch_bytes))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        for (j = 0; j < i; ++j) {
            uintptr_t a = sites[i].rva, b = sites[j].rva;
            if (a < b + sites[j].patch_bytes && b < a + sites[i].patch_bytes)
                return E_INVALIDARG;
        }
    }
    bytes = count * THUNK_BYTES;
    code = allocate_near(image, image_size, bytes);
    if (!code) return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    memset(code, 0xcc, bytes);
    for (i = 0; i < count; ++i) {
        unsigned char *thunk = code + i * THUNK_BYTES;
        int64_t relative = (int64_t)(uintptr_t)thunk -
                           (int64_t)(uintptr_t)(image + sites[i].rva + 5);
        uint32_t displacement = tls_index * 8;
        if (relative < INT32_MIN || relative > INT32_MAX) {
            VirtualFree(code, 0, MEM_RELEASE);
            return HRESULT_FROM_WIN32(ERROR_ARITHMETIC_OVERFLOW);
        }
        memcpy(thunk, sites[i].expected, 9);
        thunk[9] = 0x48;
        if (sites[i].patch_bytes == 12) {
            thunk[10] = 0x8b;
            thunk[11] = (unsigned char)(0x80 | (sites[i].lookup_destination << 3));
        } else {
            thunk[10] = 0x8d;
            thunk[11] = sites[i].destination == 0 ? 0x80 : 0x89;
        }
        memcpy(thunk + 12, &displacement, 4);
        thunk[16] = 0xc3;
    }
    if (!VirtualProtect(code, bytes, PAGE_EXECUTE_READ, &old_protect)) {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        VirtualFree(code, 0, MEM_RELEASE);
        return hr;
    }
    if (!FlushInstructionCache(GetCurrentProcess(), code, bytes)) {
        HRESULT hr = HRESULT_FROM_WIN32(GetLastError());
        VirtualFree(code, 0, MEM_RELEASE);
        return hr;
    }
    /* The loader has not started a guest thread at this boundary. */
    for (i = 0; i < count; ++i) {
        unsigned char *target = image + sites[i].rva;
        int32_t relative = (int32_t)((int64_t)(uintptr_t)(code + i * THUNK_BYTES) -
                                    (int64_t)(uintptr_t)(target + 5));
        DWORD restore;
        if (!VirtualProtect(target, sites[i].patch_bytes, PAGE_READWRITE, &restore)) {
            /* No guest code may run after a partial installation. */
            RaiseFailFastException(NULL, NULL, 0);
            return E_FAIL;
        }
        target[0] = 0xe8;
        memcpy(target + 1, &relative, 4);
        memset(target + 5, 0x90, sites[i].patch_bytes - 5);
        if (!VirtualProtect(target, sites[i].patch_bytes, restore, &old_protect) ||
            !FlushInstructionCache(GetCurrentProcess(), target, sites[i].patch_bytes)) {
            RaiseFailFastException(NULL, NULL, 0);
            return E_FAIL;
        }
    }
    redirect->code = code;
    redirect->bytes = bytes;
    redirect->count = count;
    return S_OK;
}
