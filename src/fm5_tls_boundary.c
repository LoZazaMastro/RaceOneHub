#include "fm5_guest_tls.h"
#include "audio-tls/guest_tls_sites.h"
#include <intrin.h>
#include <stdio.h>
#include <string.h>

C_ASSERT(FM5_TLS_GUEST_ALL_SITES_CLASSIFIED == 1);
C_ASSERT(FM5_TLS_GUEST_SITE_COUNT == 327);

typedef void (*NativeBinder)(void *, const void *);
static FM5TLSRedirect installed;
static SRWLOCK install_lock = SRWLOCK_INIT;
static unsigned char *installed_image;

static HRESULT redirect_after_binding(unsigned char *owner, unsigned char *host) {
    unsigned char *game;
    uint32_t size, index, host_index, exported_index;
    HMODULE helper;
    DWORD *exported;
    void **vector;
    FM5TLSRedirectSite sites[FM5_TLS_GUEST_SITE_COUNT];
    HRESULT result;
    game = *(unsigned char **)owner;
    memcpy(&size, owner + 8, 4);
    memcpy(&index, owner + 0x360, 4);
    memcpy(&helper, owner + 0x368, sizeof(helper));
    memcpy(&host_index, host + 0x329084, 4);
    if (!game || size != 0x279d000 || !helper || !index ||
        index == host_index || index > 65535)
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    exported = (DWORD *)(uintptr_t)GetProcAddress(helper, "azimuth_tls_index");
    if (!exported) return HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND);
    memcpy(&exported_index, exported, 4);
    if (exported_index != index ||
        memcmp(game + FM5_TLS_GUEST_INDEX_RVA, &index, 4))
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    vector = (void **)(uintptr_t)__readgsqword(0x58);
    if (!vector || !vector[index] || vector[index] == vector[host_index])
        return HRESULT_FROM_WIN32(ERROR_INVALID_DATA);
    AcquireSRWLockExclusive(&install_lock);
    if (installed_image) {
        result = installed_image == game ? S_FALSE : E_UNEXPECTED;
    } else {
        C_ASSERT(sizeof(sites) == sizeof(fm5_tls_guest_sites));
        memcpy(sites, fm5_tls_guest_sites, sizeof(sites));
        result = FM5RedirectGuestTLS(game, size, index, sites,
                                    FM5_TLS_GUEST_SITE_COUNT, &installed);
        if (SUCCEEDED(result)) installed_image = game;
    }
    ReleaseSRWLockExclusive(&install_lock);
    fprintf(stderr, "[fm5-tls] result=%08lx index=%lu host_index=%lu sites=%zu guest=%p thunks=%p\n",
            (unsigned long)result, (unsigned long)index, (unsigned long)host_index,
            installed.count, (void *)game, (void *)installed.code);
    return result;
}

__declspec(dllexport) void FM5BindGuestTLS(void *owner, const void *helper_path) {
    static const unsigned char binder[] = {
        0x40,0x57,0x48,0x83,0xec,0x40,0x48,0x8b,
        0xf9,0x4c,0x8b,0xca,0x8b,0x89,0xf8,0x00
    };
    unsigned char *host = (unsigned char *)GetModuleHandleW(NULL);
    HRESULT result = HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    if (owner && host && !memcmp(host + 0xbe6a0, binder, sizeof(binder))) {
        ((NativeBinder)(uintptr_t)(host + 0xbe6a0))(owner, helper_path);
        result = redirect_after_binding(owner, host);
    }
    if (FAILED(result)) {
        ULONG_PTR details[] = {(ULONG_PTR)(uint32_t)result};
        fprintf(stderr, "[fm5-tls] installation refused: %08lx\n", (unsigned long)result);
        RaiseException(0xe0f50003, EXCEPTION_NONCONTINUABLE, 1, details);
    }
}
