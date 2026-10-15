#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "audio-tls/private-data/fm5_private_data.h"

HRESULT FM5PatchResourceVtable(void **, UINT, const unsigned char *, size_t);
static unsigned factory_calls;
HRESULT FM5PatchFactoryVtable(void **table, UINT kind, const unsigned char *base, size_t size) {
    assert(table && (kind == 0xe || kind == 0xf) && base && size == 0x100000);
    ++factory_calls;
    return S_OK;
}
void FM5LegacyGetDescription(void *resource, void *description) { (void)resource; (void)description; }
HRESULT FM5GetPrivateData(void *a, REFGUID b, UINT *c, void *d) {
    (void)a; (void)b; (void)c; (void)d; return S_OK;
}
HRESULT FM5SetPrivateData(void *a, REFGUID b, UINT c, const void *d) {
    (void)a; (void)b; (void)c; (void)d; return S_OK;
}
HRESULT FM5SetPrivateDataInterface(void *a, REFGUID b, void *c) {
    (void)a; (void)b; (void)c; return S_OK;
}
HRESULT FM5SetPrivateDataInterface2(void *a, REFGUID b, void *c) {
    (void)a; (void)b; (void)c; return S_OK;
}

int main(void) {
    static const UINT kinds[] = {5, 0xc, 0xd, 0x10};
    static const UINT getters[] = {0xeab70, 0xeab90, 0xeabd0, 0xeac00};
    unsigned char *base = VirtualAlloc(NULL, 0x100000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    void *table[270], *expected[270];
    UINT i, index;
    assert(base);
    assert(FM5PatchResourceVtable(NULL, 5, base, 0x100000) == E_POINTER);
    for (i = 0; i < 4; ++i) {
        for (index = 0; index < 270; ++index) table[index] = (void *)(uintptr_t)(index + 1);
        table[4] = base + 0xf6330;
        table[5] = base + 0xf6430;
        table[6] = base + 0xf6530;
        table[7] = base + 0xf6630;
        table[11] = base + 0xf6a30;
        table[12] = base + getters[i];
        memcpy(expected, table, sizeof(table));
        assert(FM5PatchResourceVtable(table, kinds[i], base, 0xf6a30) == HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT));
        assert(!memcmp(table, expected, sizeof(table)));
        table[11] = NULL;
        assert(FAILED(FM5PatchResourceVtable(table, kinds[i], base, 0x100000)));
        table[11] = base + 0xf6a30;
        table[12] = NULL;
        assert(FAILED(FM5PatchResourceVtable(table, kinds[i], base, 0x100000)));
        table[12] = base + getters[i];
        for (index = 4; index < 8; ++index) {
            void *saved = table[index];
            table[index] = NULL;
            assert(FAILED(FM5PatchResourceVtable(table, kinds[i], base, 0x100000)));
            table[index] = saved;
            assert(!memcmp(table, expected, sizeof(table)));
        }
        expected[4] = (void *)FM5GetPrivateData;
        expected[5] = (void *)FM5SetPrivateData;
        expected[6] = (void *)FM5SetPrivateDataInterface;
        expected[7] = (void *)FM5SetPrivateDataInterface2;
        expected[11] = (void *)FM5LegacyGetDescription;
        assert(FM5PatchResourceVtable(table, kinds[i], base, 0x100000) == S_OK);
        assert(!memcmp(table, expected, sizeof(table)));
    }
    memcpy(expected, table, sizeof(table));
    assert(FM5PatchResourceVtable(table, 6, base, 0x100000) == E_INVALIDARG);
    assert(!memcmp(table, expected, sizeof(table)));
    for (index = 0; index < 270; ++index) table[index] = (void *)(uintptr_t)(index + 1);
    table[4] = base + 0xf6330;
    table[5] = base + 0xf6430;
    table[6] = base + 0xf6530;
    table[7] = base + 0xf6630;
    memcpy(expected, table, sizeof(table));
    for (index = 4; index < 8; ++index) {
        void *saved = table[index];
        table[index] = NULL;
        assert(FAILED(FM5PatchResourceVtable(table, 8, base, 0x100000)));
        table[index] = saved;
        assert(!memcmp(table, expected, sizeof(table)));
    }
    expected[4] = (void *)FM5GetPrivateData;
    expected[5] = (void *)FM5SetPrivateData;
    expected[6] = (void *)FM5SetPrivateDataInterface;
    expected[7] = (void *)FM5SetPrivateDataInterface2;
    assert(FM5PatchResourceVtable(table, 8, base, 0x100000) == S_OK);
    assert(!memcmp(table, expected, sizeof(table)));
    assert(FM5PatchResourceVtable(table, 0xe, base, 0x100000) == S_OK && factory_calls == 1);
    assert(!memcmp(table, expected, sizeof(table)));
    assert(FM5PatchResourceVtable(table, 0xf, base, 0x100000) == S_OK && factory_calls == 2);
    assert(!memcmp(table, expected, sizeof(table)));
    VirtualFree(base, 0, MEM_RELEASE);
    puts("PASS: resource/shader vtable aliases and factory delegation; unrelated entries preserved, wrong layouts rejected");
    return 0;
}
