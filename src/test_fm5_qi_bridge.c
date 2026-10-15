#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef HRESULT (*Query)(void *, const GUID *, void **);
void FM5SetQueryTestBackend(Query);
HRESULT FM5GraphicsQueryInterface(void *, const GUID *, void **);

static const GUID context_iid = {0x9458fe06,0xc78d,0x47f7,{0x96,0xa0,0xec,0x7b,0x72,0x7b,0xe1,0xe9}};
static const GUID device_iid = {0x88671610,0x712e,0x4f1e,{0x84,0xab,0x01,0xb5,0x94,0x8b,0xd3,0x73}};
static const GUID other_iid = {0};
static unsigned context_calls, device_calls, releases;
static HRESULT wrap_status, query_status;
static BOOL empty_query, empty_wrap;
static int wrapped_value;
static void *native_expected;

static ULONG release_native(void *object) {
    assert(object == native_expected);
    ++releases;
    return 1;
}
static HRESULT backend(void *object, const GUID *iid, void **output) {
    assert(object == native_expected && iid);
    *output = empty_query || FAILED(query_status) ? NULL : native_expected;
    return query_status;
}
HRESULT FM5WrapPerformanceContext(void *native_owned, void **output) {
    assert(native_owned == native_expected);
    ++context_calls;
    *output = FAILED(wrap_status) || empty_wrap ? NULL : &wrapped_value;
    return wrap_status;
}
HRESULT FM5WrapPerformanceDevice(void *native_owned, void **output) {
    assert(native_owned == native_expected);
    ++device_calls;
    *output = FAILED(wrap_status) || empty_wrap ? NULL : &wrapped_value;
    return wrap_status;
}
int main(void) {
    void *vtable[3] = {NULL, NULL, (void *)release_native};
    void **native = vtable;
    void *output;
    native_expected = &native;
    assert(FM5GraphicsQueryInterface(native_expected, &other_iid, NULL) == E_POINTER);
    assert(FM5GraphicsQueryInterface(NULL, &other_iid, &output) == E_POINTER && !output);
    assert(FM5GraphicsQueryInterface(native_expected, NULL, &output) == E_POINTER && !output);
    assert(FAILED(FM5GraphicsQueryInterface(native_expected, &other_iid, &output)) && !output);
    FM5SetQueryTestBackend(backend);
    assert(FM5GraphicsQueryInterface(native_expected, &other_iid, &output) == S_OK && output == native_expected);
    assert(!context_calls && !device_calls && !releases);
    assert(FM5GraphicsQueryInterface(native_expected, &context_iid, &output) == S_OK && output == &wrapped_value);
    assert(context_calls == 1 && !device_calls && !releases);
    assert(FM5GraphicsQueryInterface(native_expected, &device_iid, &output) == S_OK && output == &wrapped_value);
    assert(context_calls == 1 && device_calls == 1 && !releases);
    wrap_status = E_OUTOFMEMORY;
    assert(FM5GraphicsQueryInterface(native_expected, &context_iid, &output) == E_OUTOFMEMORY && !output);
    assert(releases == 1);
    wrap_status = S_OK;
    empty_wrap = TRUE;
    assert(FM5GraphicsQueryInterface(native_expected, &device_iid, &output) == E_UNEXPECTED && !output);
    assert(releases == 2);
    empty_wrap = FALSE;
    empty_query = TRUE;
    assert(FM5GraphicsQueryInterface(native_expected, &context_iid, &output) == E_UNEXPECTED && !output);
    assert(releases == 2);
    empty_query = FALSE;
    query_status = E_NOINTERFACE;
    assert(FM5GraphicsQueryInterface(native_expected, &context_iid, &output) == E_NOINTERFACE && !output);
    assert(releases == 2 && context_calls == 2 && device_calls == 2);
    puts("PASS: QI selection, original delegation, ownership transfer and failure cleanup (11 cases)");
    return 0;
}
