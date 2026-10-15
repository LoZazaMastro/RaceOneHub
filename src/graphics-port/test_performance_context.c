#include <assert.h>
#include <stdio.h>
#include "fm5_performance_context.h"

static ULONG refs = 1;
static void *expected;
static BOOL last_enabled;
static unsigned policy_calls;
static HRESULT native_query(void *self, const GUID *iid, void **out) {
    (void)iid;
    assert(self == expected);
    *out = NULL;
    return E_NOINTERFACE;
}
static ULONG native_release(void *self) {
    assert(self == expected && refs);
    return --refs;
}
static HRESULT policy(void *self, BOOL enabled) {
    assert(self == expected);
    last_enabled = enabled;
    ++policy_calls;
    return S_OK;
}
int main(void) {
    void *table[3] = {(void *)native_query, 0, (void *)native_release};
    void **native = table;
    void *a, *b;
    typedef ULONG (*Release)(void *);
    typedef void (*SetAutomaticCompletion)(void *, BOOL);
    expected = &native;
    assert(FM5WrapPerformanceContext(NULL, &a) == E_INVALIDARG && !a);
    assert(FM5WrapPerformanceContext(expected, NULL) == E_POINTER && refs == 1);
    assert(FM5WrapPerformanceContext(expected, &a) == S_OK && refs == 1);
    ++refs;
    assert(FM5WrapPerformanceContext(expected, &b) == S_OK && refs == 1 && a == b);
    assert(*(void ***)a == (void **)((char *)a + 0x20));
    FM5ConfigurePerformanceContext(policy);
    ((SetAutomaticCompletion)*(void **)((char *)a + 0x618))(a, 1);
    assert(policy_calls == 1 && last_enabled);
    ((SetAutomaticCompletion)*(void **)((char *)a + 0x618))(a, 0);
    assert(policy_calls == 2 && !last_enabled);
    assert(((Release)(*(void ***)a)[2])(a) == 1 && refs == 1);
    assert(((Release)(*(void ***)b)[2])(b) == 0 && refs == 0);
    refs = 1;
    assert(FM5WrapPerformanceContext(expected, &a) == S_OK && refs == 1);
    assert(((Release)(*(void ***)a)[2])(a) == 0 && refs == 0);
    puts("PASS: inline ABI, BOOL policy forwarding, ownership adoption, cache hit reference consumption, final cleanup");
    return 0;
}
