#include "fm5_queue_diagnostics.h"
#include "../allocation-port/fm5_luminance_readback.h"
#pragma warning(push)
#pragma warning(disable: 4201)
#include <d3d12.h>
#pragma warning(pop)
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdarg.h>

struct ListHistory {
    const void *list;
    const void *queue;
    const void *fence;
    UINT64 sequence;
    UINT64 batch;
};
struct ListObservation {
    const void *list;
    ListHistory previous;
    BOOL duplicate;
    BOOL pending;
    BOOL cross_queue;
};
static INIT_ONCE initialization = INIT_ONCE_STATIC_INIT;
static SRWLOCK history_lock = SRWLOCK_INIT;
static ListHistory history[512];
static UINT history_cursor;
static UINT64 batch_counter;
static volatile LONG ordinary_logs, suspect_logs;
static BOOL enabled;

static BOOL CALLBACK initialize(PINIT_ONCE once, void *parameter, void **context) {
    (void)once; (void)parameter; (void)context;
    WCHAR value[2] = {};
    enabled = GetEnvironmentVariableW(L"FM5_QUEUE_DIAGNOSTICS", value, 2) == 1 && value[0] == L'1';
#ifndef FM5_QUEUE_DIAGNOSTICS_TEST
    const BYTE *base = (const BYTE *)GetModuleHandleW(NULL);
    static const BYTE signature[] = {
        0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x74,0x24,0x18,0x48,0x89,0x7c,0x24,0x20,0x55,
        0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0xac,0x24,0x40,0xfe,0xff,0xff
    };
    if (enabled) {
        const IMAGE_NT_HEADERS64 *pe = base ?
            (const IMAGE_NT_HEADERS64 *)(base + ((const IMAGE_DOS_HEADER *)base)->e_lfanew) : NULL;
        if (!pe || pe->Signature != IMAGE_NT_SIGNATURE ||
            pe->OptionalHeader.SizeOfImage < 0xf1a70 + sizeof(signature) ||
            memcmp(base + 0xf1a70, signature, sizeof(signature))) enabled = FALSE;
    }
#endif
    return TRUE;
}

static void append(char *line, SIZE_T size, SIZE_T *used, const char *format, ...) {
    if (*used >= size - 1) return;
    va_list arguments;
    va_start(arguments, format);
    int written = _vsnprintf_s(line + *used, size - *used, _TRUNCATE, format, arguments);
    va_end(arguments);
    if (written < 0) *used = size - 1;
    else *used += (SIZE_T)written;
}

extern "C" __declspec(dllexport) void WINAPI FM5DiagnosticExecuteCommandLists(
    void *raw_queue, UINT count, void *const *raw_lists, void *owner, UINT64 sequence) {
    DWORD last_error = GetLastError();
    ID3D12CommandQueue *queue = (ID3D12CommandQueue *)raw_queue;
    InitOnceExecuteOnce(&initialization, initialize, NULL, NULL);
    if (enabled && owner && *(void **)((BYTE *)owner + 0x10) == raw_queue && raw_lists) {
        ID3D12Fence *fence = *(ID3D12Fence **)((BYTE *)owner + 0x18);
        UINT64 completed = fence ? fence->GetCompletedValue() : UINT64_MAX;
        ListObservation observed[64] = {};
        UINT captured = count < _countof(observed) ? count : (UINT)_countof(observed);
        UINT duplicate_count = 0, pending_count = 0, cross_count = 0;
        UINT64 batch;
        AcquireSRWLockExclusive(&history_lock);
        batch = ++batch_counter;
        for (UINT i = 0; i < captured; ++i) {
            ListObservation *o = &observed[i];
            o->list = raw_lists[i];
            for (UINT j = 0; j < i; ++j) if (observed[j].list == o->list) {
                o->duplicate = TRUE;
                ++duplicate_count;
                break;
            }
            for (UINT j = 0; j < _countof(history); ++j) {
                const ListHistory *entry = &history[j];
                if (entry->list == o->list && entry->batch > o->previous.batch)
                    o->previous = *entry;
            }
            if (o->previous.list) {
                o->cross_queue = o->previous.queue != raw_queue;
                o->pending = fence && o->previous.fence == fence &&
                    completed != UINT64_MAX && o->previous.sequence > completed;
                if (o->pending) ++pending_count;
                if (o->cross_queue) ++cross_count;
            }
        }
        /* Record only after gathering: duplicates in this batch must not be
         * confused with a previous Execute from a different batch. No COM
         * pointers are retained/queried later, and no lifetime is extended. */
        for (UINT i = 0; i < captured; ++i) {
            ListHistory *entry = &history[history_cursor++ % _countof(history)];
            entry->list = observed[i].list; entry->queue = raw_queue; entry->fence = fence;
            entry->sequence = sequence; entry->batch = batch;
        }
        ReleaseSRWLockExclusive(&history_lock);
        BOOL suspect = duplicate_count || pending_count || cross_count;
        LONG log_number = suspect ? InterlockedIncrement(&suspect_logs) : InterlockedIncrement(&ordinary_logs);
        if (log_number <= (suspect ? 32 : 8)) {
            char line[20000];
            SIZE_T used = 0;
            LARGE_INTEGER timestamp = {};
            QueryPerformanceCounter(&timestamp);
            append(line, sizeof(line), &used,
                "{\"event\":\"fm5_execute_batch\",\"thread\":%lu,\"qpc\":%lld,"
                "\"batch\":%llu,\"queue\":\"0x%llx\",\"owner\":\"0x%llx\","
                "\"fence\":\"0x%llx\",\"sequence\":%llu,\"completed\":%llu,"
                "\"count\":%u,\"captured\":%u,\"duplicate_entries\":%u,"
                "\"previous_pending\":%u,\"cross_queue\":%u,\"lists\":[",
                GetCurrentThreadId(), timestamp.QuadPart, batch,
                (unsigned long long)(uintptr_t)raw_queue, (unsigned long long)(uintptr_t)owner,
                (unsigned long long)(uintptr_t)fence, sequence, completed, count, captured,
                duplicate_count, pending_count, cross_count);
            for (UINT i = 0; i < captured; ++i) {
                const ListObservation *o = &observed[i];
                append(line, sizeof(line), &used,
                    "%s{\"pointer\":\"0x%llx\",\"duplicate\":%u,\"pending\":%u,"
                    "\"previous_batch\":%llu,\"previous_queue\":\"0x%llx\","
                    "\"previous_fence\":\"0x%llx\",\"previous_sequence\":%llu}",
                    i ? "," : "", (unsigned long long)(uintptr_t)o->list,
                    (UINT)o->duplicate, (UINT)o->pending, o->previous.batch,
                    (unsigned long long)(uintptr_t)o->previous.queue,
                    (unsigned long long)(uintptr_t)o->previous.fence, o->previous.sequence);
            }
            append(line, sizeof(line), &used, "]}\n");
            if (used < sizeof(line) - 1) FM5DiagnosticWriteJsonLine(line, (DWORD)used);
        }
    }
    SetLastError(last_error);
    /* Exact original submission. Do not filter duplicate entries, wait on any
     * fence, reset lists, alter deadlines or translate the native API result. */
    queue->ExecuteCommandLists(count, (ID3D12CommandList *const *)raw_lists);
    /* A private readback fence can be signaled only after the real submission.
     * No queue-history lock is held. Genuine submission exceptions propagate
     * normally and cannot reach this callback. The optional module returns
     * immediately when its independent diagnostic flag is disabled. */
    DWORD submission_error = GetLastError();
    FM5ReadbackSubmitted(raw_queue, count, raw_lists);
    SetLastError(submission_error);
}
