#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <intrin.h>
#include <stdio.h>
#include <stdint.h>

static DWORD *helper_index;
static volatile LONG ready;
static HANDLE release_workers;

static unsigned char *block(void) {
    void **slots = (void **)__readgsqword(0x58);
    return slots[*helper_index];
}

static DWORD WINAPI worker(void *arg) {
    unsigned char value = (unsigned char)(uintptr_t)arg;
    unsigned char *p = block();
    if (!p || ((uintptr_t)p & 15)) return 1;
    for (unsigned i = 0; i < 336; ++i) if (p[i]) return 2;
    p[0] = value;
    p[39] = (unsigned char)(value + 1);
    InterlockedIncrement(&ready);
    if (WaitForSingleObject(release_workers, 10000) != WAIT_OBJECT_0) return 3;
    return p[0] != value || p[39] != (unsigned char)(value + 1) ? 4 : 0;
}

int wmain(int argc, wchar_t **argv) {
    if (argc != 2) return 2;
    HMODULE helper = LoadLibraryExW(argv[1], NULL,
        LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!helper) { printf("load_error=%lu\n", GetLastError()); return 3; }
    helper_index = (DWORD *)GetProcAddress(helper, "azimuth_tls_index");
    DWORD *attached = (DWORD *)GetProcAddress(helper, "azimuth_tls_attached");
    if (!helper_index || *helper_index == (DWORD)-1 || !attached || *attached != GetCurrentThreadId()) return 4;
    unsigned char *p = block();
    if (!p || ((uintptr_t)p & 15)) return 5;
    for (unsigned i = 0; i < 336; ++i) if (p[i]) return 6;
    p[0] = 0xf0; p[39] = 0xf1;
    release_workers = CreateEventW(NULL, TRUE, FALSE, NULL);
    if (!release_workers) return 7;
    HANDLE threads[4];
    for (uintptr_t i = 0; i < 4; ++i) {
        threads[i] = CreateThread(NULL, 0, worker, (void *)(i + 1), 0, NULL);
        if (!threads[i]) return 8;
    }
    for (unsigned timeout = 0; ready != 4 && timeout < 10000; ++timeout) Sleep(1);
    SetEvent(release_workers);
    if (ready != 4 || WaitForMultipleObjects(4, threads, TRUE, 10000) != WAIT_OBJECT_0) return 9;
    for (unsigned i = 0; i < 4; ++i) {
        DWORD code;
        if (!GetExitCodeThread(threads[i], &code) || code) return 10;
        CloseHandle(threads[i]);
    }
    if (p[0] != 0xf0 || p[39] != 0xf1) return 11;
    CloseHandle(release_workers);
    printf("{\"passed\":true,\"threads\":5,\"helper_bytes\":336,\"game_bytes\":40,\"alignment\":16,\"index\":%lu}\n", *helper_index);
    FreeLibrary(helper);
    return 0;
}
