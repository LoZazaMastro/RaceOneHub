#include "fm5_frame_timing.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef DWORD (WINAPI *WaitBackend)(HANDLE, DWORD);
typedef LONGLONG (WINAPI *DeadlineBackend)(void *, LONGLONG, LONGLONG,
    LONGLONG, LONGLONG);

static const BYTE deadline_signature[32] = {
    0x4c,0x8b,0xdc,0x49,0x89,0x5b,0x10,0x49,0x89,0x73,0x18,0x49,0x89,0x7b,0x20,0x41,
    0x56,0x48,0x81,0xec,0xc0,0x00,0x00,0x00,0x45,0x33,0xf6,0x49,0x8b,0xf8,0x48,0x8b
};

typedef struct TimingCounter {
    ULONGLONG calls;
    LONGLONG total_ticks;
    LONGLONG max_ticks;
} TimingCounter;

typedef struct ThreadTiming {
    LONGLONG started;
    TimingCounter latency;
    TimingCounter timer;
    TimingCounter deadline;
    LONGLONG last_delay;
    LONGLONG last_period;
    LONGLONG last_display_period;
    LONGLONG last_phase;
} ThreadTiming;

static __declspec(thread) ThreadTiming thread_timing;
static INIT_ONCE timing_once = INIT_ONCE_STATIC_INIT;
static BOOL timing_enabled;
static LONGLONG timing_frequency;
static WaitBackend timing_wait;
static DeadlineBackend timing_deadline;
static const volatile UINT *native_fps;

static HRESULT validate_host(const BYTE *base, SIZE_T size) {
    if (!base) return E_POINTER;
    if (size < 0x4fe30 + sizeof(deadline_signature) ||
        memcmp(base + 0x4fe30, deadline_signature, sizeof(deadline_signature)))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    return S_OK;
}

static BOOL CALLBACK initialize_timing(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    LARGE_INTEGER frequency;
    WCHAR value[3];
    (void)once; (void)parameter; (void)context;
    timing_wait = WaitForSingleObject;
    timing_enabled = GetEnvironmentVariableW(L"FM5_FRAME_TIMING", value, 3) == 1 &&
        value[0] == L'1';
    if (timing_enabled && QueryPerformanceFrequency(&frequency))
        timing_frequency = frequency.QuadPart;
    if (timing_frequency <= 0) timing_enabled = FALSE;
    return TRUE;
}

static BOOL enabled(void) {
    InitOnceExecuteOnce(&timing_once, initialize_timing, NULL, NULL);
    return timing_enabled;
}

#ifndef FM5_FRAME_TIMING_TEST
static INIT_ONCE deadline_once = INIT_ONCE_STATIC_INIT;
static BOOL CALLBACK initialize_deadline(PINIT_ONCE once, PVOID parameter, PVOID *context) {
    const BYTE *base = (const BYTE *)GetModuleHandleW(NULL);
    const IMAGE_NT_HEADERS64 *pe;
    (void)once; (void)parameter; (void)context;
    if (!base) { SetLastError(ERROR_BAD_EXE_FORMAT); return FALSE; }
    pe = (const IMAGE_NT_HEADERS64 *)(base + ((const IMAGE_DOS_HEADER *)base)->e_lfanew);
    if (pe->Signature != IMAGE_NT_SIGNATURE ||
        pe->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        FAILED(validate_host(base, pe->OptionalHeader.SizeOfImage))) {
        SetLastError(ERROR_BAD_EXE_FORMAT);
        return FALSE;
    }
    timing_deadline = (DeadlineBackend)(base + 0x4fe30);
    if (pe->OptionalHeader.SizeOfImage >= 0x318e98 + sizeof(UINT))
        native_fps = (const volatile UINT *)(base + 0x318e98);
    return TRUE;
}
#endif

static LONGLONG ticks_now(void) {
    LARGE_INTEGER value;
    return QueryPerformanceCounter(&value) ? value.QuadPart : 0;
}

static double microseconds(LONGLONG ticks) {
    return timing_frequency > 0 ? (double)ticks * 1000000.0 / (double)timing_frequency : 0.0;
}

static void count_ticks(TimingCounter *counter, LONGLONG ticks) {
    if (ticks < 0) ticks = 0;
    ++counter->calls;
    counter->total_ticks += ticks;
    if (ticks > counter->max_ticks) counter->max_ticks = ticks;
}

static void emit_if_due(LONGLONG now) {
    ThreadTiming *s = &thread_timing;
    char line[1024];
    int length;
    DWORD written;
    if (!s->started) { s->started = now; return; }
    if (now - s->started < timing_frequency) return;
    length = _snprintf_s(line, sizeof(line), _TRUNCATE,
        "{\"event\":\"fm5_frame_timing\",\"thread\":%lu,\"window_us\":%.3f,"
        "\"latency_calls\":%llu,\"latency_total_us\":%.3f,\"latency_max_us\":%.3f,"
        "\"timer_calls\":%llu,\"timer_total_us\":%.3f,\"timer_max_us\":%.3f,"
        "\"deadline_calls\":%llu,\"deadline_total_us\":%.3f,"
        "\"last_deadline_delay_us\":%.3f,\"last_period_us\":%.3f,"
        "\"last_display_period_us\":%.3f,\"last_phase_ticks\":%lld,\"native_fps\":%u}\n",
        GetCurrentThreadId(), microseconds(now - s->started),
        s->latency.calls, microseconds(s->latency.total_ticks), microseconds(s->latency.max_ticks),
        s->timer.calls, microseconds(s->timer.total_ticks), microseconds(s->timer.max_ticks),
        s->deadline.calls, microseconds(s->deadline.total_ticks),
        microseconds(s->last_delay), microseconds(s->last_period),
        microseconds(s->last_display_period), s->last_phase, native_fps ? *native_fps : 0);
    if (length > 0)
        WriteFile(GetStdHandle(STD_ERROR_HANDLE), line, (DWORD)length, &written, NULL);
    memset(s, 0, sizeof(*s));
    s->started = now;
}

static DWORD measured_wait(HANDLE object, DWORD milliseconds, BOOL timer) {
    DWORD before_error = GetLastError();
    DWORD result, after_error;
    LONGLONG before, after;
    if (!enabled()) {
        SetLastError(before_error);
        return timing_wait(object, milliseconds);
    }
    before = ticks_now();
    SetLastError(before_error);
    result = timing_wait(object, milliseconds);
    after_error = GetLastError();
    after = ticks_now();
    if (before && after) {
        count_ticks(timer ? &thread_timing.timer : &thread_timing.latency, after - before);
        emit_if_due(after);
    }
    SetLastError(after_error);
    return result;
}

__declspec(dllexport) DWORD WINAPI FM5FrameLatencyWait(HANDLE object, DWORD milliseconds) {
    return measured_wait(object, milliseconds, FALSE);
}

__declspec(dllexport) DWORD WINAPI FM5PacingTimerWait(HANDLE object, DWORD milliseconds) {
    return measured_wait(object, milliseconds, TRUE);
}

__declspec(dllexport) LONGLONG WINAPI FM5PacingDeadline(void *state, LONGLONG now,
    LONGLONG period, LONGLONG display_period, LONGLONG phase) {
    DWORD before_error = GetLastError();
    DWORD after_error;
    LONGLONG result, before, after;
    BOOL collect;
#ifndef FM5_FRAME_TIMING_TEST
    if (!InitOnceExecuteOnce(&deadline_once, initialize_deadline, NULL, NULL)) {
        ULONG_PTR detail = GetLastError();
        RaiseException(0xe0f50030, EXCEPTION_NONCONTINUABLE, 1, &detail);
        return 0;
    }
#endif
    collect = enabled();
    before = collect ? ticks_now() : 0;
    SetLastError(before_error);
    result = timing_deadline(state, now, period, display_period, phase);
    after_error = GetLastError();
    after = collect ? ticks_now() : 0;
    if (collect && before && after) {
        count_ticks(&thread_timing.deadline, after - before);
        thread_timing.last_delay = result - now;
        thread_timing.last_period = period;
        thread_timing.last_display_period = display_period;
        thread_timing.last_phase = phase;
        emit_if_due(after);
    }
    SetLastError(after_error);
    return result;
}

#ifdef FM5_FRAME_TIMING_TEST
void FM5FrameTimingTestConfigure(FM5TimingWaitBackend wait_backend,
    FM5TimingDeadlineBackend deadline_backend, BOOL collect) {
    LARGE_INTEGER frequency;
    InitOnceExecuteOnce(&timing_once, initialize_timing, NULL, NULL);
    timing_wait = wait_backend;
    timing_deadline = deadline_backend;
    timing_enabled = collect;
    timing_frequency = QueryPerformanceFrequency(&frequency) ? frequency.QuadPart : 0;
    memset(&thread_timing, 0, sizeof(thread_timing));
}
HRESULT FM5FrameTimingTestGuard(const BYTE *base, SIZE_T size) {
    return validate_host(base, size);
}
#endif
