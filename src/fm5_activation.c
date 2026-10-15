#include "fm5_activation.h"
#include <stdio.h>
#include <wchar.h>

static FM5ActivationBackend original_activation;
static SRWLOCK activation_lock = SRWLOCK_INIT;
static volatile LONG speech_messages;

HRESULT FM5ConfigureActivation(FM5ActivationBackend backend) {
    HRESULT result = S_OK;
    if (!backend || backend == FM5GetActivationFactory) return E_INVALIDARG;
    AcquireSRWLockExclusive(&activation_lock);
    if (original_activation && original_activation != backend) result = E_UNEXPECTED;
    else original_activation = backend;
    ReleaseSRWLockExclusive(&activation_lock);
    return result;
}

HRESULT WINAPI FM5GetActivationFactory(const wchar_t *name, const GUID *iid, void **out) {
    FM5ActivationBackend backend;
    if (!name || !iid) return E_INVALIDARG;
    if (!out) return E_POINTER;
    if (!wcscmp(name, L"Windows.Xbox.Speech.Recognition.SpeechRecognizer")) {
        *out = NULL;
        if (InterlockedIncrement(&speech_messages) == 1)
            fprintf(stderr, "[fm5-winrt] Xbox speech unavailable: HRESULT=80040154; game handles optional service absence\n");
        return REGDB_E_CLASSNOTREG;
    }
    AcquireSRWLockShared(&activation_lock);
    backend = original_activation;
    ReleaseSRWLockShared(&activation_lock);
    return backend ? backend(name, iid, out) : E_UNEXPECTED;
}
