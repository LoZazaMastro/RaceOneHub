#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>

typedef struct FM5TLSRedirectSite {
    uint32_t rva;
    uint8_t destination;
    uint8_t patch_bytes;
    uint8_t lookup_destination;
    uint8_t expected[12];
} FM5TLSRedirectSite;

typedef struct FM5TLSRedirect {
    unsigned char *code;
    size_t bytes;
    size_t count;
} FM5TLSRedirect;

HRESULT FM5RedirectGuestTLS(unsigned char *image, size_t image_size,
    DWORD tls_index, const FM5TLSRedirectSite *sites, size_t count,
    FM5TLSRedirect *redirect);
