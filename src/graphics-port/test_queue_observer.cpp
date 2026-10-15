#include "fm5_queue_diagnostics.h"
#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>

extern "C" void FM5QueueHookAbiProbe(void *, UINT, void *const *, void *, UINT64);
extern "C" void FM5QueueHookCandidateStub(void);
static void *wanted_queue;
static UINT wanted_count, executions, fence_queries;
static UINT readback_callbacks;
static BOOL throw_submission;
static void *const *wanted_lists;
static UINT64 completed;
static void WINAPI execute_recorder(void *queue, UINT count, void *const *lists) {
    assert(queue == wanted_queue && count == wanted_count && lists == wanted_lists);
    assert(GetLastError() == 0x1234);
    void *stack[12];
    USHORT frames = CaptureStackBackTrace(0, 12, stack, NULL);
    DWORD64 image = 0;
    PRUNTIME_FUNCTION stub = RtlLookupFunctionEntry((DWORD64)(uintptr_t)FM5QueueHookCandidateStub, &image, NULL);
    assert(stub);
    BOOL found_stub = FALSE;
    for (USHORT i = 0; i < frames; ++i) {
        DWORD64 pc = (DWORD64)(uintptr_t)stack[i];
        if (pc >= image + stub->BeginAddress && pc < image + stub->EndAddress) found_stub = TRUE;
    }
    assert(found_stub);
    ++executions;
    if (throw_submission) RaiseException(0xe1234501, 0, 0, NULL);
    SetLastError(0x5678);
}
extern "C" void WINAPI FM5ReadbackSubmitted(void *queue, UINT count, void *const *lists) {
    assert(queue == wanted_queue && count == wanted_count && lists == wanted_lists);
    assert(GetLastError() == 0x5678);
    assert(executions == readback_callbacks + 1);
    ++readback_callbacks;
    SetLastError(0x9999); /* Must not replace the genuine Execute LastError. */
}
static BOOL expect_submission_exception(void *queue, UINT count, void *const *lists,
                                       void *owner, UINT64 sequence) {
    __try {
        FM5QueueHookAbiProbe(queue, count, lists, owner, sequence);
    } __except (GetExceptionCode() == 0xe1234501 ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        return TRUE;
    }
    return FALSE;
}
static UINT64 WINAPI completed_recorder(void *) {
    ++fence_queries;
    SetLastError(0xabcdef);
    return completed;
}

int main(int argc, char **argv) {
    assert(argc == 2);
    BOOL enabled = strcmp(argv[1], "enabled") == 0;
    SetEnvironmentVariableW(L"FM5_QUEUE_DIAGNOSTICS", enabled ? L"1" : L"0");
    void *queue_table[11] = {};
    void *fence_table[9] = {};
    void **queue = queue_table;
    void **fence = fence_table;
    void **queue2 = queue_table;
    BYTE owner[32] = {};
    queue_table[10] = (void *)execute_recorder;
    fence_table[8] = (void *)completed_recorder;
    *(void **)(owner + 0x10) = &queue;
    *(void **)(owner + 0x18) = &fence;
    void *lists[64];
    for (UINT i = 0; i < 64; ++i) lists[i] = (void *)(uintptr_t)(0x1234567800000000ULL + i * 16);
    lists[2] = lists[0];
    void *original_lists[64];
    memcpy(original_lists, lists, sizeof(lists));
    wanted_queue = &queue; wanted_lists = lists;
    UINT64 sequence = 0x1122334455667788ULL;
    for (UINT count = 0; count < 65; ++count) {
        wanted_count = count;
        SetLastError(0x1234);
        FM5QueueHookAbiProbe(&queue, count, lists, owner, sequence++);
        assert(GetLastError() == 0x5678);
    }
    completed = UINT64_MAX;
    wanted_count = 1;
    SetLastError(0x1234);
    FM5QueueHookAbiProbe(&queue, wanted_count, lists, owner, sequence++);
    assert(GetLastError() == 0x5678);
    wanted_queue = &queue2;
    *(void **)(owner + 0x10) = &queue2;
    SetLastError(0x1234);
    FM5QueueHookAbiProbe(&queue2, wanted_count, lists, owner, sequence++);
    assert(GetLastError() == 0x5678);
    assert(executions == 67);
    assert(readback_callbacks == executions);
    assert(fence_queries == (enabled ? 67U : 0U));
    throw_submission = TRUE;
    SetLastError(0x1234);
    assert(expect_submission_exception(&queue2, wanted_count, lists, owner, sequence++));
    assert(executions == 68 && readback_callbacks == 67);
    assert(memcmp(original_lists, lists, sizeof(lists)) == 0);
    printf("PASS queue observer %s:67 unchanged submissions, ordered readback callbacks, native exception propagation, raw arrays, stack fiftharg, LastError\n", argv[1]);
    return 0;
}
