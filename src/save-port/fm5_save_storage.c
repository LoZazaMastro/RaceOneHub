#include "fm5_save_storage.h"
#include <shlobj.h>
#include <knownfolders.h>
#include <string.h>
#include <wchar.h>

typedef struct HostString {
    union { WCHAR inline_buffer[8]; WCHAR *heap; } data;
    SIZE_T length;
    SIZE_T capacity;
} HostString;
typedef void *(WINAPI *Assign)(void *, const WCHAR *, SIZE_T);
typedef void (WINAPI *Destroy)(void *);
typedef void *(WINAPI *BuildTitle)(void *, const void *, const void *);
struct FM5SaveStoragePlan {
    const BYTE *host;
    SIZE_T host_size;
    HostString root;
    Assign assign;
    Destroy destroy;
    BuildTitle build;
};
typedef char HostStringMustBe32[sizeof(HostString) == 32 ? 1 : -1];
static FM5SaveStoragePlan *volatile active;
static const WCHAR product_suffix[] = L"\\Race One Hub\\RaceOne5";
typedef struct Guard { SIZE_T offset; BYTE bytes[32]; SIZE_T length; } Guard;
static const Guard guards[] = {
    {0x81dc0,{0x40,0x55,0x53,0x56,0x57,0x41,0x56,0x48,0x8d,0x6c,0x24,0xc9,0x48,0x81,0xec,0xf0,
        0,0,0,0x48,0x8b,0x05,0x66,0x64,0x29,0,0x48,0x33,0xc4,0x48,0x89,0x45},32},
    {0x7bd0,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x6c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x57},16},
    {0x8d50,{0x40,0x53,0x48,0x83,0xec,0x30,0x48,0x8b,0x51,0x18,0x48,0x8b,0xd9,0x48,0x83,0xfa},16},
    /* Argument setup stays original even when root patches the following CALL. */
    {0x1870b9,{0x4c,0x8d,0x05,0x88,0x39,0x19,0,0x48,0x8d,0x15,0xf9,0x38,0x19,0,0x48,0x8d,0x4d,0x2f},18},
    {0x818d0,{0x48,0x89,0x5c,0x24,0x18,0x48,0x89,0x74,0x24,0x20,0x57,0x48,0x83,0xec,0x70,0x48},16}
};

static const WCHAR *characters(const HostString *value) {
    return value->capacity > 7 ? value->data.heap : value->data.inline_buffer;
}
static HRESULT validate(const BYTE *host, SIZE_T size) {
    SIZE_T i;
    if (!host) return E_POINTER;
    if (size < 0x32b5f1) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    for (i = 0; i < sizeof(guards) / sizeof(guards[0]); ++i)
        if (memcmp(host + guards[i].offset, guards[i].bytes, guards[i].length))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    /* Debug mode appends data-debug; this adapter intentionally supports data. */
    if (host[0x32b5f0]) return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
    return S_OK;
}

#ifdef FM5_SAVE_STORAGE_TEST
static Assign test_assign;
static Destroy test_destroy;
static BuildTitle test_build;
void FM5SaveStorageTestBackends(FM5SaveAssign assign, FM5SaveDestroy destroy, FM5SaveBuildTitle build) {
    test_assign = assign; test_destroy = destroy; test_build = build;
}
#endif

static HRESULT prepare_path(const BYTE *host, SIZE_T size, const WCHAR *known,
    FM5SaveStoragePlan **out) {
    FM5SaveStoragePlan *plan;
    WCHAR *path;
    SIZE_T length;
    HRESULT status;
    if (!out) return E_POINTER;
    *out = NULL;
    status = validate(host, size);
    if (FAILED(status)) return status;
    if (!known || !known[0]) return HRESULT_FROM_WIN32(ERROR_PATH_NOT_FOUND);
    length = wcslen(known);
    while (length && (known[length - 1] == L'\\' || known[length - 1] == L'/')) --length;
    if (!length || length > 32000) return HRESULT_FROM_WIN32(ERROR_FILENAME_EXCED_RANGE);
    plan = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*plan));
    if (!plan) return E_OUTOFMEMORY;
    plan->root.capacity = 7;
    plan->host = host;
    plan->host_size = size;
#ifdef FM5_SAVE_STORAGE_TEST
    plan->assign = test_assign; plan->destroy = test_destroy; plan->build = test_build;
#else
    plan->assign = (Assign)(host + 0x7bd0);
    plan->destroy = (Destroy)(host + 0x8d50);
    plan->build = (BuildTitle)(host + 0x81dc0);
#endif
    if (!plan->assign || !plan->destroy || !plan->build) {
        HeapFree(GetProcessHeap(), 0, plan); return E_UNEXPECTED;
    }
    path = HeapAlloc(GetProcessHeap(), 0, (length + _countof(product_suffix)) * sizeof(WCHAR));
    if (!path) { HeapFree(GetProcessHeap(), 0, plan); return E_OUTOFMEMORY; }
    memcpy(path, known, length * sizeof(WCHAR));
    memcpy(path + length, product_suffix, sizeof(product_suffix));
    __try {
        plan->assign(&plan->root, path, length + _countof(product_suffix) - 1);
        *out = plan;
    } __finally {
        HeapFree(GetProcessHeap(), 0, path);
        if (!*out) {
            plan->destroy(&plan->root);
            HeapFree(GetProcessHeap(), 0, plan);
        }
    }
    return S_OK;
}

HRESULT FM5PrepareSaveStorage(const BYTE *host, SIZE_T size, FM5SaveStoragePlan **out) {
    PWSTR known = NULL;
    HRESULT status;
    if (!out) return E_POINTER;
    *out = NULL;
    status = validate(host, size);
    if (FAILED(status)) return status;
    status = SHGetKnownFolderPath(&FOLDERID_SavedGames, 0, NULL, &known);
    if (FAILED(status)) return status;
    __try { status = prepare_path(host, size, known, out); }
    __finally { CoTaskMemFree(known); }
    return status;
}

static HRESULT probe_path(const FM5SaveStoragePlan *plan) {
    static const WCHAR tail[] = L"\\data\\titles\\7B35D917";
    HostString id = {0}, output = {0};
    WCHAR *expected;
    SIZE_T length = plan->root.length + _countof(tail) - 1;
    HRESULT status = E_FAIL;
    id.capacity = output.capacity = 7;
    expected = HeapAlloc(GetProcessHeap(), 0, (length + 1) * sizeof(WCHAR));
    if (!expected) return E_OUTOFMEMORY;
    memcpy(expected, characters(&plan->root), plan->root.length * sizeof(WCHAR));
    memcpy(expected + plan->root.length, tail, sizeof(tail));
    __try {
        plan->assign(&id, L"7B35D917", 8);
        if (plan->build(&output, &plan->root, &id) == &output && output.length == length &&
            !memcmp(characters(&output), expected, (length + 1) * sizeof(WCHAR)))
            status = S_OK;
    } __finally {
        plan->destroy(&output);
        plan->destroy(&id);
        HeapFree(GetProcessHeap(), 0, expected);
    }
    return status;
}

HRESULT FM5ProbePreparedSaveStorage(const FM5SaveStoragePlan *plan) {
    HRESULT status;
    if (!plan) return E_POINTER;
    status = validate(plan->host, plan->host_size);
    if (FAILED(status)) return status;
    /* Diagnostic callers receive failure explicitly even for a native helper
       exception. Both temporary native strings are cleaned by probe_path. */
    __try { return probe_path(plan); }
    __except(EXCEPTION_EXECUTE_HANDLER) {
        return HRESULT_FROM_NT(GetExceptionCode());
    }
}

void FM5DiscardSaveStorage(FM5SaveStoragePlan **value) {
    FM5SaveStoragePlan *plan;
    if (!value || !*value) return;
    plan = *value; *value = NULL;
    plan->destroy(&plan->root);
    HeapFree(GetProcessHeap(), 0, plan);
}
void FM5CommitSaveStorage(FM5SaveStoragePlan **value) {
    if (!value || !*value) return;
    if (InterlockedCompareExchangePointer((PVOID volatile *)&active, *value, NULL))
        FM5DiscardSaveStorage(value);
    else *value = NULL;
}

static void fatal(HRESULT status) {
    ULONG_PTR detail = (ULONG_PTR)(ULONG)status;
    RaiseException(0xe0f50042, EXCEPTION_NONCONTINUABLE, 1, &detail);
}
__declspec(dllexport) void *WINAPI FM5BuildPersistentSaveTitlePath(
    void *output, const void *original_root, const void *title_id) {
    FM5SaveStoragePlan *plan = InterlockedCompareExchangePointer((PVOID volatile *)&active, NULL, NULL);
    const HostString *id = title_id;
    void *result;
    int error;
    DWORD attributes;
    if (!plan || !output || original_root != plan->host + 0x31a9c0 ||
        title_id != plan->host + 0x31aa48 || id->length != 8 ||
        _wcsnicmp(characters(id), L"7B35D917", 8)) {
        fatal(HRESULT_FROM_WIN32(ERROR_INVALID_STATE)); return NULL;
    }
    result = plan->build(output, &plan->root, title_id);
    if (result != output) {
        fatal(E_UNEXPECTED); return NULL;
    }
    error = SHCreateDirectoryExW(NULL, characters((const HostString *)output), NULL);
    attributes = GetFileAttributesW(characters((const HostString *)output));
    if ((error != ERROR_SUCCESS && error != ERROR_ALREADY_EXISTS && error != ERROR_FILE_EXISTS) ||
        attributes == INVALID_FILE_ATTRIBUTES || !(attributes & FILE_ATTRIBUTE_DIRECTORY)) {
        HRESULT status = HRESULT_FROM_WIN32(error ? (DWORD)error : ERROR_DIRECTORY);
        plan->destroy(output);
        fatal(status); return NULL;
    }
    return result;
}

#ifdef FM5_SAVE_STORAGE_TEST
HRESULT FM5SaveStorageTestPrepare(const BYTE *host, SIZE_T size, const WCHAR *path, FM5SaveStoragePlan **out) {
    return prepare_path(host, size, path, out);
}
void FM5SaveStorageTestReset(void) {
    FM5SaveStoragePlan *plan = InterlockedExchangePointer((PVOID volatile *)&active, NULL);
    FM5DiscardSaveStorage(&plan);
}
HRESULT FM5SaveStorageTestGuard(const BYTE *host, SIZE_T size) { return validate(host, size); }
#endif
