#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>

__declspec(dllexport) __declspec(noinline) void FM5FixtureBreakTarget(void *address) {
    *(volatile unsigned int *)address = 0x12345678;
}

int wmain(int argc, wchar_t **argv) {
    if (argc > 1 && !wcscmp(argv[1], L"--wait")) {
        Sleep(INFINITE);
        return 0;
    }
    FM5FixtureBreakTarget(NULL);
    return 0;
}
