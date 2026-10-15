#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>

void FM5RunResourceDestruction(void *, void (*)(void *));
static unsigned sequence;
static void *expected;
void FM5ForgetPrivateData(void *resource) {
    assert(resource == expected && sequence == 0);
    sequence = 1;
}
void FM5ForgetLegacyResource(void *resource) {
    assert(resource == expected && sequence == 1);
    sequence = 2;
}
static void destructor(void *resource) {
    assert(resource == expected && sequence == 2);
    sequence = 3;
}
int main(void) {
    int resource;
    expected = &resource;
    FM5RunResourceDestruction(expected, destructor);
    assert(sequence == 3);
    puts("PASS: private data and resource metadata removed before native destruction; exact object forwarded once");
    return 0;
}
