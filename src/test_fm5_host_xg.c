#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

HRESULT WINAPI FM5CreateHostTextureComputer(const void *, void **, void *);
static HRESULT WINAPI wrong_factory(const void *desc, void **out) {
    (void)desc; (void)out;
    assert(!"Unverified native factory must not be called");
    return E_FAIL;
}

int wmain(int argc, wchar_t **argv) {
    DWORD source[16], before[16];
    UINT checked = 0;
    assert(argc == 4);
    void *object = (void *)1;
    assert(FAILED(FM5CreateHostTextureComputer(source, &object, (void *)wrong_factory)) && !object);
    HMODULE kernel = LoadLibraryExW(argv[1], NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    assert(kernel);
    for (UINT library = 2; library <= 3; ++library) {
        HMODULE module = LoadLibraryExW(argv[library], NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
        assert(module);
        for (UINT dimension = 2; dimension <= 3; ++dimension) {
            union { FARPROC proc; void *address; } factory;
            factory.proc = GetProcAddress(module, dimension == 2 ? "XGCreateTexture2DComputer" : "XGCreateTexture3DComputer");
            assert(factory.proc);
            memset(source, 0, sizeof(source));
            source[0] = source[1] = 64;
            if (dimension == 2) {
                source[2] = 7; source[3] = 1; source[4] = 99;
                source[5] = 1; source[8] = 8; source[13] = 14;
            } else {
                source[2] = 16; source[3] = 1; source[4] = 28;
                source[6] = 8; source[11] = 14;
            }
            memcpy(before, source, sizeof(source));
            assert(FM5CreateHostTextureComputer(source, &object, factory.address) == S_OK && object);
            assert(!memcmp(before, source, sizeof(source)));
            HMODULE modern = GetModuleHandleW(L"fm5-host-xg.dll");
            assert(modern && *(void **)object == (BYTE *)modern + 0x14670);
            union {
                void *address;
                ULONG (WINAPI *reference)(void *);
                HRESULT (WINAPI *layout)(void *, void *);
                UINT64 (WINAPI *offset)(void *, UINT, UINT);
                UINT64 (WINAPI *element)(void *, UINT, UINT, UINT, UINT, UINT, UINT);
                HRESULT (WINAPI *copy)(void *, void *, UINT, UINT, const void *, UINT, UINT);
            } method;
            BYTE layout[0xd40 + 32];
            memset(layout, 0xa5, sizeof(layout));
            method.address = (*(void ***)object)[2];
            assert(method.layout(object, layout) == S_OK);
            for (UINT i = 0xd40; i < sizeof(layout); ++i) assert(layout[i] == 0xa5);
            UINT64 resource_bytes;
            memcpy(&resource_bytes, layout, sizeof(resource_bytes));
            assert(resource_bytes > 0);
            method.address = (*(void ***)object)[3];
            UINT64 mip_offset = method.offset(object, 0, 0);
            assert(mip_offset < resource_bytes);
            method.address = (*(void ***)object)[4];
            UINT64 first = method.element(object, 0, 0, 0, 0, 0, 0);
            UINT64 second = method.element(object, 0, 0, 1, 0, 0, 0);
            assert(first < resource_bytes && second < resource_bytes && first != second);
            for (UINT mip = 0; mip < (dimension == 2 ? 7u : 1u); ++mip) {
                UINT width = 64u >> mip, height = 64u >> mip;
                UINT rows = dimension == 2 ? (height + 3) / 4 : height;
                UINT elements = dimension == 2 ? (width + 3) / 4 : width;
                UINT element_bytes = dimension == 2 ? 16 : 4;
                UINT depth = dimension == 2 ? 1 : 16;
                UINT row_bytes = elements * element_bytes;
                UINT row_pitch = row_bytes + 16;
                UINT slice_pitch = row_pitch * rows + 32;
                SIZE_T linear_size = (SIZE_T)slice_pitch * depth;
                BYTE *input = HeapAlloc(GetProcessHeap(), 0, linear_size + 32);
                BYTE *tiled = HeapAlloc(GetProcessHeap(), 0, (SIZE_T)resource_bytes + 32);
                BYTE *detiled = HeapAlloc(GetProcessHeap(), 0, linear_size + 32);
                assert(input && tiled && detiled);
                memset(input, 0xcc, linear_size + 32);
                memset(tiled, 0x7a, (SIZE_T)resource_bytes + 32);
                memset(detiled, 0xa5, linear_size + 32);
                for (UINT z = 0; z < depth; ++z)
                    for (UINT y = 0; y < rows; ++y)
                        for (UINT x = 0; x < row_bytes; ++x)
                            input[(SIZE_T)z * slice_pitch + (SIZE_T)y * row_pitch + x] =
                                (BYTE)(x * 17 + y * 31 + z * 7 + mip * 19);
                method.address = (*(void ***)object)[5];
                assert(method.copy(object, tiled, 0, mip, input, row_pitch, slice_pitch) == S_OK);
                method.address = (*(void ***)object)[6];
                assert(method.copy(object, detiled, 0, mip, tiled, row_pitch, slice_pitch) == S_OK);
                for (UINT z = 0; z < depth; ++z) {
                    for (UINT y = 0; y < rows; ++y) {
                        SIZE_T at = (SIZE_T)z * slice_pitch + (SIZE_T)y * row_pitch;
                        assert(!memcmp(input + at, detiled + at, row_bytes));
                        for (UINT x = row_bytes; x < row_pitch; ++x)
                            assert(detiled[at + x] == 0xa5);
                    }
                    for (UINT x = row_pitch * rows; x < slice_pitch; ++x)
                        assert(detiled[(SIZE_T)z * slice_pitch + x] == 0xa5);
                }
                for (UINT x = 0; x < 32; ++x) {
                    assert(detiled[linear_size + x] == 0xa5);
                    assert(tiled[(SIZE_T)resource_bytes + x] == 0x7a);
                }
                HeapFree(GetProcessHeap(), 0, input);
                HeapFree(GetProcessHeap(), 0, tiled);
                HeapFree(GetProcessHeap(), 0, detiled);
            }
            method.address = (*(void ***)object)[0];
            assert(method.reference(object) == 2);
            method.address = (*(void ***)object)[1];
            assert(method.reference(object) == 1);
            assert(method.reference(object) == 0);
            ++checked;
        }
        FreeLibrary(module);
    }
    FreeLibrary(kernel);
    printf("PASS: %u host factory routes; source preserved, genuine modern8-slot interface, compact layout guards, mip/texel offsets, tile/untile roundtrip, padded rows/slices, canaries, reference lifecycle and wrong target rejection\n", checked);
    return 0;
}
