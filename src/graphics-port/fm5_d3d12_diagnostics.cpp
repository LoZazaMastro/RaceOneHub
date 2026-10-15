#include "fm5_d3d12_diagnostics.h"
#pragma warning(push)
#pragma warning(disable: 4201)
#include <d3d12.h>
#include <d3d12sdklayers.h>
#pragma warning(pop)
#include <stdint.h>
#include <stdio.h>
#include <tlhelp32.h>

static INIT_ONCE initialization = INIT_ONCE_STATIC_INIT;
static SRWLOCK log_lock = SRWLOCK_INIT;
typedef HRESULT (WINAPI *CreateDeviceFn)(void *, UINT, const GUID *, void **);
static CreateDeviceFn original_create;
static HMODULE system_runtime;
static BOOL enabled;
static SRWLOCK module_lock = SRWLOCK_INIT;
static volatile LONG sync_stack_samples;
struct ModuleRange {
    uintptr_t base;
    SIZE_T size;
    char name[80];
};
static ModuleRange module_ranges[128];
static UINT module_count;

/* Enumerate outside the InfoQueue callback. The callback only reads this fixed
 * snapshot: no D3D calls, module loading or loader enumeration while reporting. */
static void capture_module_ranges(void) {
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
    if (snapshot == INVALID_HANDLE_VALUE) return;
    MODULEENTRY32W entry = {};
    entry.dwSize = sizeof(entry);
    AcquireSRWLockExclusive(&module_lock);
    module_count = 0;
    if (Module32FirstW(snapshot, &entry)) do {
        if (module_count == _countof(module_ranges)) break;
        ModuleRange *range = &module_ranges[module_count++];
        range->base = (uintptr_t)entry.modBaseAddr;
        range->size = entry.modBaseSize;
        UINT i = 0;
        while (entry.szModule[i] && i + 1 < _countof(range->name)) {
            WCHAR ch = entry.szModule[i];
            range->name[i] = ch >= 32 && ch < 127 && ch != '"' && ch != '\\' ? (char)ch : '?';
            ++i;
        }
        range->name[i] = 0;
    } while (Module32NextW(snapshot, &entry));
    ReleaseSRWLockExclusive(&module_lock);
    CloseHandle(snapshot);
}

static void write_bytes(const char *text, DWORD length) {
    DWORD written;
    HANDLE destination = GetStdHandle(STD_ERROR_HANDLE);
    if (destination && destination != INVALID_HANDLE_VALUE)
        WriteFile(destination, text, length, &written, NULL);
}

extern "C" void FM5DiagnosticWriteJsonLine(const char *text, DWORD length) {
    AcquireSRWLockExclusive(&log_lock);
    write_bytes(text, length);
    ReleaseSRWLockExclusive(&log_lock);
}

static void log_status(const char *event_name, HRESULT hr) {
    char line[256];
    int length = _snprintf_s(line, sizeof(line), _TRUNCATE,
        "{\"event\":\"%s\",\"hr\":\"0x%08lx\",\"thread\":%lu}\n",
        event_name, (unsigned long)hr, GetCurrentThreadId());
    if (length > 0) {
        AcquireSRWLockExclusive(&log_lock);
        write_bytes(line, (DWORD)length);
        ReleaseSRWLockExclusive(&log_lock);
    }
}

static void capture_sync_stack(void) {
    LONG sample = InterlockedIncrement(&sync_stack_samples);
    if (sample > 8) return;
    void *frames[24];
    USHORT count = CaptureStackBackTrace(0, (DWORD)_countof(frames), frames, NULL);
    LARGE_INTEGER timestamp = {};
    QueryPerformanceCounter(&timestamp);
    char line[320];
    int length = _snprintf_s(line, sizeof(line), _TRUNCATE,
        "{\"event\":\"d3d12_command_list_sync_stack\",\"id\":553,\"sample\":%ld,"
        "\"thread\":%lu,\"qpc\":%lld,\"frames\":[",
        sample, GetCurrentThreadId(), timestamp.QuadPart);
    AcquireSRWLockShared(&module_lock);
    AcquireSRWLockExclusive(&log_lock);
    if (length > 0) write_bytes(line, (DWORD)length);
    for (USHORT i = 0; i < count; ++i) {
        uintptr_t address = (uintptr_t)frames[i];
        const ModuleRange *found = NULL;
        for (UINT j = 0; j < module_count; ++j) {
            const ModuleRange *range = &module_ranges[j];
            if (address >= range->base && address - range->base < range->size) {
                found = range;
                break;
            }
        }
        length = _snprintf_s(line, sizeof(line), _TRUNCATE,
            "%s{\"address\":\"0x%llx\",\"module\":\"%s\",\"offset\":\"0x%llx\"}",
            i ? "," : "", (unsigned long long)address,
            found ? found->name : "unknown", (unsigned long long)(found ? address - found->base : address));
        if (length > 0) write_bytes(line, (DWORD)length);
    }
    write_bytes("]}\n", 3);
    ReleaseSRWLockExclusive(&log_lock);
    ReleaseSRWLockShared(&module_lock);
}

static void CALLBACK on_message(D3D12_MESSAGE_CATEGORY category,
    D3D12_MESSAGE_SEVERITY severity, D3D12_MESSAGE_ID id,
    LPCSTR description, void *context) {
    (void)context;
    if (severity != D3D12_MESSAGE_SEVERITY_CORRUPTION &&
        severity != D3D12_MESSAGE_SEVERITY_ERROR) return;
    char line[1024];
    int length = _snprintf_s(line, sizeof(line), _TRUNCATE,
        "{\"event\":\"d3d12_message\",\"severity\":%u,\"category\":%u,\"id\":%u,\"thread\":%lu,\"description\":\"",
        (unsigned)severity, (unsigned)category, (unsigned)id, GetCurrentThreadId());
    AcquireSRWLockExclusive(&log_lock);
    if (length > 0) write_bytes(line, (DWORD)length);
    DWORD used = 0;
    if (description) for (const unsigned char *p = (const unsigned char *)description; *p; ++p) {
        static const char hex[] = "0123456789abcdef";
        if (used > sizeof(line) - 7) { write_bytes(line, used); used = 0; }
        if (*p == '"' || *p == '\\') { line[used++] = '\\'; line[used++] = (char)*p; }
        else if (*p < 32) {
            line[used++] = '\\'; line[used++] = 'u'; line[used++] = '0'; line[used++] = '0';
            line[used++] = hex[*p >> 4]; line[used++] = hex[*p & 15];
        } else line[used++] = (char)*p;
    }
    write_bytes(line, used);
    write_bytes("\"}\n", 3);
    ReleaseSRWLockExclusive(&log_lock);
    if (id == D3D12_MESSAGE_ID_COMMAND_LIST_SYNC) capture_sync_stack();
}

static BOOL CALLBACK initialize(PINIT_ONCE once, void *parameter, void **context) {
    (void)once; (void)parameter; (void)context;
    system_runtime = LoadLibraryExW(L"d3d12.dll", NULL, LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!system_runtime) return FALSE;
    original_create = (CreateDeviceFn)(uintptr_t)GetProcAddress(system_runtime, "D3D12CreateDevice");
    if (!original_create) return FALSE;
    WCHAR value[2] = {0};
    enabled = GetEnvironmentVariableW(L"FM5_D3D12_DEBUG", value, 2) == 1 && value[0] == L'1';
    if (enabled) {
        auto get_debug = (PFN_D3D12_GET_DEBUG_INTERFACE)(uintptr_t)
            GetProcAddress(system_runtime, "D3D12GetDebugInterface");
        ID3D12Debug *debug = NULL;
        HRESULT hr = get_debug ? get_debug(__uuidof(ID3D12Debug), (void **)&debug) : E_NOINTERFACE;
        if (SUCCEEDED(hr) && debug) { debug->EnableDebugLayer(); debug->Release(); }
        else if (SUCCEEDED(hr)) hr = E_UNEXPECTED;
        log_status("d3d12_debug_enable", hr);
    }
    return TRUE;
}

extern "C" __declspec(dllexport) HRESULT WINAPI FM5DiagnosticD3D12CreateDevice(
    void *adapter, UINT minimum_feature_level, const GUID *iid, void **out) {
    if (!InitOnceExecuteOnce(&initialization, initialize, NULL, NULL))
        return HRESULT_FROM_WIN32(GetLastError());
    /* The actual runtime validates IID/output, including its NULL-output probe.
     * Do not replace outputs or translate the native HRESULT. */
    HRESULT hr = original_create(adapter, minimum_feature_level, iid, out);
    if (enabled && SUCCEEDED(hr) && out && *out) {
        ID3D12InfoQueue1 *queue = NULL;
        HRESULT attached = ((IUnknown *)*out)->QueryInterface(__uuidof(ID3D12InfoQueue1), (void **)&queue);
        if (SUCCEEDED(attached) && queue) {
            DWORD cookie = 0;
            capture_module_ranges();
            attached = queue->RegisterMessageCallback(on_message,
                D3D12_MESSAGE_CALLBACK_IGNORE_FILTERS, NULL, &cookie);
            /* The queue/device owns registration. Callback/context are static
             * in this imported bridge, which remains loaded for process life.
             * Keeping another device reference would introduce a lifetime leak. */
            queue->Release();
        } else if (SUCCEEDED(attached)) attached = E_UNEXPECTED;
        log_status("d3d12_infoqueue_callback", attached);
    }
    return hr;
}
