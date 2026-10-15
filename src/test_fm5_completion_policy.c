#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

int FM5VerifyCompletionSetter(const unsigned char *, size_t, const void *);
void FM5SetCompletionTestBackend(unsigned char *, size_t, void (*)(void *, BOOL));
HRESULT FM5ApplyNativeCompletionPolicy(void *, BOOL);
static unsigned calls;
static BOOL last;
static void *expected_native;
static void setter(void *native, BOOL enabled) {
    assert(native == expected_native);
    ++calls;
    last = enabled;
}
int main(void) {
    static const unsigned char signature[] = {
        0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,
        0x48,0x83,0xec,0x60,0x8b,0xea,0x48,0x8b,0xd9,0x80,0xb9,0x48,0x0c,0x00,0x00,0x00
    };
    static const unsigned char recording_signature[] = {
        0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x40,0x8b,0xda,0x48,0x8b,0xf9,0x48,
        0x8b,0xd1,0x48,0x8d,0x4c,0x24,0x30,0xe8,0xa4,0x45,0xfa,0xff,0xc7,0x44,0x24,0x20
    };
    unsigned char *image = VirtualAlloc(NULL, 0x130000, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    void *table[0xc5] = {0};
    void **native = table;
    unsigned index;
    assert(image);
    memcpy(image + 0x12cb90, signature, sizeof(signature));
    memcpy(image + 0x11e940, recording_signature, sizeof(recording_signature));
    assert(FM5VerifyCompletionSetter(image, 0x130000, image + 0x12cb90));
    assert(!FM5VerifyCompletionSetter(NULL, 0x130000, image + 0x12cb90));
    assert(!FM5VerifyCompletionSetter(image, 0x12cb90, image + 0x12cb90));
    assert(!FM5VerifyCompletionSetter(image, 0x130000, image + 0x12cb91));
    for (index = 0; index < sizeof(signature); ++index) {
        image[0x12cb90 + index] ^= 1;
        assert(!FM5VerifyCompletionSetter(image, 0x130000, image + 0x12cb90));
        image[0x12cb90 + index] ^= 1;
    }
    assert(FM5VerifyCompletionSetter(image, 0x130000, image + 0x11e940));
    assert(!FM5VerifyCompletionSetter(image, 0x11e940, image + 0x11e940));
    for (index = 0; index < sizeof(recording_signature); ++index) {
        image[0x11e940 + index] ^= 1;
        assert(!FM5VerifyCompletionSetter(image, 0x130000, image + 0x11e940));
        image[0x11e940 + index] ^= 1;
    }
    table[0xc4] = (void *)setter;
    expected_native = &native;
    FM5SetCompletionTestBackend(image, 0x130000, setter);
    assert(FM5ApplyNativeCompletionPolicy(NULL, TRUE) == E_POINTER);
    assert(FM5ApplyNativeCompletionPolicy(expected_native, 2) == S_OK && calls == 1 && last == TRUE);
    assert(FM5ApplyNativeCompletionPolicy(expected_native, FALSE) == S_OK && calls == 2 && last == FALSE);
    table[0xc4] = NULL;
    assert(FAILED(FM5ApplyNativeCompletionPolicy(expected_native, TRUE)) && calls == 2);
    VirtualFree(image, 0, MEM_RELEASE);
    puts("PASS: completion policy dispatch, BOOL normalization, two native backends and 64 signature mutations");
    return 0;
}
