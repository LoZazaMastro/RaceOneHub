#include "fm5_activation.h"
#include <assert.h>
#include <stdio.h>
#include <wchar.h>

static unsigned calls;
static int object;
static GUID expected = {0x35,0,0,{0xc0,0,0,0,0,0,0,0x46}};
static HRESULT WINAPI backend(const wchar_t *name,const GUID *iid,void **out) {
    assert(!wcscmp(name,L"Windows.Xbox.System.User"));
    assert(iid==&expected);
    ++calls;
    *out=&object;
    return S_FALSE;
}
static HRESULT WINAPI wrong_backend(const wchar_t *name,const GUID *iid,void **out) {
    (void)name;(void)iid;(void)out;
    assert(!"Unconfigured backend invoked");return E_FAIL;
}
int main(void) {
    void *out=(void *)1;
    assert(FM5ConfigureActivation(NULL)==E_INVALIDARG);
    assert(FM5ConfigureActivation(FM5GetActivationFactory)==E_INVALIDARG);
    assert(FM5GetActivationFactory(NULL,&expected,&out)==E_INVALIDARG);
    assert(FM5GetActivationFactory(L"x",NULL,&out)==E_INVALIDARG);
    assert(FM5GetActivationFactory(L"x",&expected,NULL)==E_POINTER);
    assert(FM5GetActivationFactory(L"Windows.Xbox.System.User",&expected,&out)==E_UNEXPECTED);
    assert(FM5ConfigureActivation(backend)==S_OK);
    assert(FM5ConfigureActivation(backend)==S_OK);
    assert(FM5ConfigureActivation(wrong_backend)==E_UNEXPECTED);
    assert(FM5GetActivationFactory(L"Windows.Xbox.Speech.Recognition.SpeechRecognizer",&expected,&out)==REGDB_E_CLASSNOTREG);
    assert(!out && !calls);
    assert(FM5GetActivationFactory(L"Windows.Xbox.System.User",&expected,&out)==S_FALSE);
    assert(out==&object && calls==1);
    puts("PASS: exact optional speech absence, NULL output, validation, backend identity and unchanged activation delegation");
    return 0;
}
