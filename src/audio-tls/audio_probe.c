#include "fm5_audio_bridge.h"
#include <stdio.h>
#include <stdint.h>

static unsigned calls;
static HRESULT result;
static int object;
static int abi_ok;
__declspec(noinline) static HRESULT WINAPI record(void **out, UINT32 flags, UINT32 processor, void *context) {
    ++calls;
    abi_ok = out != NULL && flags == 0 && processor == 0xffffffffu && context == NULL;
    if (SUCCEEDED(result)) *out = &object;
    return result;
}

#define CHECK(x) do { if (!(x)) { printf("failed_line=%u\n", __LINE__); return 1; } } while (0)
int main(void) {
    void *out = (void *)(uintptr_t)1;
    FM5AudioSetTestBackend(record);
    result = S_OK;
    CHECK(FM5XAudio2Create(&out, 0, 0x400, NULL) == S_OK);
    CHECK(out == &object && calls == 1 && abi_ok);
    CHECK(FM5XAudio2Create(NULL, 0, 0x400, NULL) == E_POINTER && calls == 1);
    CHECK(FM5XAudio2Create(&out, 1, 0x400, NULL) == E_INVALIDARG && out == NULL && calls == 1);
    CHECK(FM5XAudio2Create(&out, 0x400, 0x400, NULL) == E_INVALIDARG && calls == 1);
    CHECK(FM5XAudio2Create(&out, 0, 0xffffffffu, NULL) == E_INVALIDARG && calls == 1);
    CHECK(FM5XAudio2Create(&out, 0, 0x400, &object) == E_INVALIDARG && calls == 1);
    result = (HRESULT)0x88960004;
    CHECK(FM5XAudio2Create(&out, 0, 0x400, NULL) == result && out == NULL && calls == 2 && abi_ok);
    FM5AudioSetTestBackend(NULL);
    CHECK(FM5XAudio2Create(&out, 0, 0x400, NULL) == HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && out == NULL && calls == 2);
    puts("{\"passed\":true,\"cases\":8,\"abi\":\"Windows x64 four arguments\",\"desktop_audio_executed\":false}");
    return 0;
}
