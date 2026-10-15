#include "fm5_frame_timing.h"
#include <stdio.h>
#include <string.h>
#include <stdint.h>

static unsigned int checks;
#define CHECK(x) do { ++checks; if (!(x)) { fprintf(stderr,"line%d: %s\n",__LINE__,#x); ExitProcess(1); } } while (0)
static HANDLE expected_handle;
static DWORD expected_timeout, expected_result;
static DWORD wait_calls, deadline_calls;
static void *expected_state;
static LONGLONG expected_now, expected_period, expected_display, expected_phase, expected_deadline;

static DWORD WINAPI wait_recorder(HANDLE object, DWORD milliseconds) {
    CHECK(object == expected_handle);
    CHECK(milliseconds == expected_timeout);
    CHECK(GetLastError() == 0x1234);
    ++wait_calls;
    SetLastError(0x5678);
    return expected_result;
}

static LONGLONG WINAPI deadline_recorder(void *state, LONGLONG now,
    LONGLONG period, LONGLONG display_period, LONGLONG phase) {
    CHECK(state == expected_state);
    CHECK(now == expected_now);
    CHECK(period == expected_period);
    CHECK(display_period == expected_display);
    CHECK(phase == expected_phase);
    CHECK(GetLastError() == 0x1234);
    ++deadline_calls;
    SetLastError(0xabcd);
    return expected_deadline;
}

int main(void) {
    BYTE signature[32] = {
        0x4c,0x8b,0xdc,0x49,0x89,0x5b,0x10,0x49,0x89,0x73,0x18,0x49,0x89,0x7b,0x20,0x41,
        0x56,0x48,0x81,0xec,0xc0,0x00,0x00,0x00,0x45,0x33,0xf6,0x49,0x8b,0xf8,0x48,0x8b
    };
    SIZE_T host_size = 0x4fe30 + sizeof(signature);
    BYTE *host = (BYTE *)VirtualAlloc(NULL, host_size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    HANDLE event;
    CHECK(host != NULL);
    memcpy(host + 0x4fe30, signature, sizeof(signature));
    CHECK(FM5FrameTimingTestGuard(NULL, host_size) == E_POINTER);
    CHECK(FAILED(FM5FrameTimingTestGuard(host, host_size - 1)));
    CHECK(SUCCEEDED(FM5FrameTimingTestGuard(host, host_size)));
    for (UINT i = 0; i < sizeof(signature); ++i) {
        host[0x4fe30 + i] ^= 1;
        CHECK(FAILED(FM5FrameTimingTestGuard(host, host_size)));
        host[0x4fe30 + i] ^= 1;
    }
    VirtualFree(host, 0, MEM_RELEASE);
    expected_handle = (HANDLE)(uintptr_t)0xfedcba9876543210ULL;
    expected_timeout = 0x87654321;
    expected_state = (void *)(uintptr_t)0x1234567887654321ULL;
    expected_now = 0x123456781234;
    expected_period = 0x567812348765;
    expected_display = 0x87654321;
    expected_phase = -0x123456781234;
    expected_deadline = expected_now + 83333;
    for (UINT enabled = 0; enabled < 2; ++enabled) {
        FM5FrameTimingTestConfigure(wait_recorder, deadline_recorder, (BOOL)enabled);
        for (UINT i = 0; i < 4; ++i) {
            static const DWORD results[4] = { WAIT_OBJECT_0, WAIT_TIMEOUT, WAIT_ABANDONED, WAIT_FAILED };
            expected_result = results[i];
            SetLastError(0x1234);
            CHECK(FM5FrameLatencyWait(expected_handle, expected_timeout) == expected_result);
            CHECK(GetLastError() == 0x5678);
            SetLastError(0x1234);
            CHECK(FM5PacingTimerWait(expected_handle, expected_timeout) == expected_result);
            CHECK(GetLastError() == 0x5678);
        }
        SetLastError(0x1234);
        CHECK(FM5PacingDeadline(expected_state, expected_now, expected_period,
            expected_display, expected_phase) == expected_deadline);
        CHECK(GetLastError() == 0xabcd);
    }
    CHECK(wait_calls == 16);
    CHECK(deadline_calls == 2);
    event = CreateEventW(NULL, TRUE, FALSE, NULL);
    CHECK(event != NULL);
    FM5FrameTimingTestConfigure(WaitForSingleObject, deadline_recorder, TRUE);
    CHECK(FM5FrameLatencyWait(event, 0) == WAIT_TIMEOUT);
    CHECK(SetEvent(event));
    CHECK(FM5PacingTimerWait(event, 0) == WAIT_OBJECT_0);
    CHECK(CloseHandle(event));
    printf("frame timing observer: %u checks PASS\n", checks);
    return 0;
}
