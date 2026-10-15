#include "fm5_presentation.h"
#include <assert.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>

static UINT expected_count;
static UINT calls;
static HRESULT result;
static const FM5LegacyPresentParameters *expected;
static void *const *expected_chains;
static BOOL expected_null;

static HRESULT WINAPI recorder(UINT sync, UINT threshold, UINT flags, UINT count,
    void *const *chains, const FM5NativePresentParameters *parameters) {
    assert(sync == 0xfedcba98u && threshold == 0x87654321u && flags == 0xa5b6c7d8u);
    assert(count == expected_count && chains == expected_chains);
    if (expected_null) assert(!parameters);
    else {
        assert(parameters);
        for (UINT i = 0; i < count; ++i) {
            assert(!memcmp(&parameters[i].legacy, &expected[i], sizeof(expected[i])));
            assert(parameters[i].flags == 0);
        }
        if (count > 1) assert(FM5PresentationTestAllocationBalance() == 1);
    }
    ++calls;
    return result;
}

static HRESULT present(UINT count, const FM5LegacyPresentParameters *parameters) {
    expected_count = count;
    expected = parameters;
    expected_null = !parameters;
    return FM5DXGIXPresentArray(0xfedcba98u, 0x87654321u, 0xa5b6c7d8u,
        count, expected_chains, parameters);
}

int main(void) {
    static const BYTE signature[32] = {
        0x40,0x53,0x56,0x57,0x48,0x81,0xec,0x30,0x01,0x00,0x00,0x48,0x8b,0x05,0x2e,0x73,
        0x2c,0x00,0x48,0x33,0xc4,0x48,0x89,0x84,0x24,0x20,0x01,0x00,0x00,0x41,0x8b,0xd8
    };
    struct { FM5LegacyPresentParameters parameters[3]; DWORD canary; } data;
    void *chains[3] = {(void *)(UINT_PTR)0x1122334455667788,
        (void *)(UINT_PTR)0x123456789abcdef0, (void *)(UINT_PTR)0x8877665544332211};
    BYTE before[sizeof(data)];
    BYTE *image = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, 0x51000);
    BYTE *pages = VirtualAlloc(NULL, 8192, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    DWORD old;
    assert(image && pages);
    expected_chains = chains;
    memset(&data, 0x6d, sizeof(data));
    for (UINT i = 0; i < 3; ++i) {
        data.parameters[i].disable = (BOOL)(i + 7);
        data.parameters[i].use_previous_buffer = (BOOL)(i + 13);
        data.parameters[i].source_rect.left = -(LONG)i - 1;
        data.parameters[i].scale_vertical = (FLOAT)i + 0.25f;
        data.parameters[i].scale_horizontal = (FLOAT)i + 0.5f;
        data.parameters[i].cookie = chains[i];
    }
    data.canary = 2;
    memcpy(before, &data, sizeof(data));
    assert(present(1, data.parameters) == E_UNEXPECTED);
    FM5PresentationTestBackend(recorder);
    result = S_OK;
    assert(present(1, data.parameters) == S_OK);
    assert(present(2, data.parameters) == S_OK);
    assert(present(3, data.parameters) == S_OK);
    assert(!memcmp(before, &data, sizeof(data)));
    assert(!FM5PresentationTestAllocationBalance());
    result = E_FAIL;
    assert(present(3, data.parameters) == E_FAIL);
    assert(!FM5PresentationTestAllocationBalance());
    result = S_FALSE;
    assert(present(1, NULL) == S_FALSE);
    assert(present(3, NULL) == S_FALSE);
    assert(present(0, (const FM5LegacyPresentParameters *)(UINT_PTR)1) == S_FALSE);
    UINT calls_before = calls;
    FM5PresentationTestFailAllocation(TRUE);
    assert(present(2, data.parameters) == E_OUTOFMEMORY);
    assert(present(UINT_MAX, data.parameters) == E_OUTOFMEMORY);
    assert(calls == calls_before && !FM5PresentationTestAllocationBalance());
    FM5PresentationTestFailAllocation(FALSE);
    /* The next DWORD is inaccessible: this exercises exactly the old48B read. */
    FM5LegacyPresentParameters *edge = (FM5LegacyPresentParameters *)(pages + 4096 - 48);
    memcpy(edge, data.parameters, 48);
    assert(VirtualProtect(pages + 4096, 4096, PAGE_NOACCESS, &old));
    result = S_OK;
    assert(present(1, edge) == S_OK);
    /* Explicit regression: adjacent DWORD2 must not become native Flags2. */
    BYTE adjacent[56];
    memcpy(adjacent, data.parameters, 48);
    memset(adjacent + 48, 0xff, 8);
    *(DWORD *)(adjacent + 48) = 2;
    assert(present(1, (const FM5LegacyPresentParameters *)adjacent) == S_OK);
    memcpy(image + 0x50f00, signature, sizeof(signature));
    assert(FM5PresentationTestGuard(image, 0x51000) == S_OK);
    assert(FAILED(FM5PresentationTestGuard(image, 0x50f00 + 31)));
    assert(FM5PresentationTestGuard(NULL, 0x51000) == E_POINTER);
    for (UINT i = 0; i < sizeof(signature); ++i) {
        image[0x50f00 + i] ^= 1;
        assert(FAILED(FM5PresentationTestGuard(image, 0x51000)));
        image[0x50f00 + i] ^= 1;
    }
    VirtualFree(pages, 0, MEM_RELEASE);
    HeapFree(GetProcessHeap(), 0, image);
    puts("PASS presentation: old48/new56, inaccessible adjacent Flags, arrays2/3, six arguments, HRESULT/NULL/OOM/cleanup and32 guard mutations");
    return 0;
}
