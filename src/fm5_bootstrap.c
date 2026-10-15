#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <wchar.h>

#define FM5_PATH_CAPACITY 32768u
#define FM5_ENV_CAPACITY (1024u * 1024u)

static BOOL fm5_file_exists(const WCHAR *path)
{
    DWORD attributes = GetFileAttributesW(path);
    return attributes != INVALID_FILE_ATTRIBUTES && !(attributes & FILE_ATTRIBUTE_DIRECTORY);
}

static WCHAR *fm5_clean_environment(void)
{
    WCHAR *original = GetEnvironmentStringsW();
    WCHAR *result, *write;
    const WCHAR *entry;
    SIZE_T characters = 0;
    if (!original) return NULL;
    for (entry = original; *entry; ) {
        SIZE_T length = wcslen(entry) + 1;
        if (length > FM5_ENV_CAPACITY - characters) {
            FreeEnvironmentStringsW(original);
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return NULL;
        }
        characters += length;
        entry += length;
    }
    result = HeapAlloc(GetProcessHeap(), 0, (characters + 2) * sizeof(WCHAR));
    if (!result) {
        FreeEnvironmentStringsW(original);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return NULL;
    }
    write = result;
    for (entry = original; *entry; ) {
        SIZE_T length = wcslen(entry) + 1;
        if (_wcsnicmp(entry, L"FM5_", 4) != 0) {
            CopyMemory(write, entry, length * sizeof(WCHAR));
            write += length;
        }
        entry += length;
    }
    *write = L'\0';
    if (write == result) write[1] = L'\0';
    FreeEnvironmentStringsW(original);
    return result;
}

static int fm5_fail(BOOL check_only, const WCHAR *message, DWORD code)
{
    if (!check_only) MessageBoxW(NULL, message, L"Forza Motorsport 5", MB_OK | MB_ICONERROR);
    return code ? (int)code : ERROR_GEN_FAILURE;
}

int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command_line, int show)
{
    WCHAR *own = NULL, *runtime = NULL, *child = NULL, *ini = NULL, *command = NULL;
    WCHAR *environment = NULL, *slash;
    WCHAR **arguments;
    DWORD length, code = ERROR_GEN_FAILURE;
    SIZE_T directory_length;
    int argument_count = 0, result = ERROR_GEN_FAILURE;
    BOOL check_only = FALSE;
    BOOL show_pre_game = FALSE;
    STARTUPINFOW startup = {0};
    PROCESS_INFORMATION process = {0};
    (void)instance; (void)previous; (void)command_line; (void)show;
    arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
    if (!arguments) return fm5_fail(FALSE, L"Unable to read the application arguments.", GetLastError());
    if (argument_count == 2 && wcscmp(arguments[1], L"--check-layout") == 0) check_only = TRUE;
    else if (argument_count != 1) {
        LocalFree(arguments);
        return fm5_fail(FALSE, L"This application does not support those launch arguments.", ERROR_BAD_ARGUMENTS);
    }
    LocalFree(arguments);
    own = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FM5_PATH_CAPACITY * sizeof(WCHAR));
    runtime = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FM5_PATH_CAPACITY * sizeof(WCHAR));
    child = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FM5_PATH_CAPACITY * sizeof(WCHAR));
    ini = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, FM5_PATH_CAPACITY * sizeof(WCHAR));
    command = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (FM5_PATH_CAPACITY + 32) * sizeof(WCHAR));
    if (!own || !runtime || !child || !ini || !command) {
        result = fm5_fail(check_only, L"Not enough memory to start the game.", ERROR_NOT_ENOUGH_MEMORY);
        goto cleanup;
    }
    length = GetModuleFileNameW(NULL, own, FM5_PATH_CAPACITY);
    if (!length || length >= FM5_PATH_CAPACITY) {
        result = fm5_fail(check_only, L"Unable to locate this application. Try a shorter folder path.", ERROR_FILENAME_EXCED_RANGE);
        goto cleanup;
    }
    slash = wcsrchr(own, L'\\');
    if (!slash) {
        result = fm5_fail(check_only, L"Unable to locate the game folder.", ERROR_PATH_NOT_FOUND);
        goto cleanup;
    }
    *slash = L'\0';
    directory_length = wcslen(own);
    if (directory_length + wcslen(L"\\runtime\\data\\RaceOneEngine.ini") >= FM5_PATH_CAPACITY ||
        directory_length + wcslen(L"\\runtime\\ForzaMastro5.PreGame.exe") >= FM5_PATH_CAPACITY) {
        result = fm5_fail(check_only, L"The game folder path is too long. Move it to a shorter path.", ERROR_FILENAME_EXCED_RANGE);
        goto cleanup;
    }
    wcscpy_s(ini, FM5_PATH_CAPACITY, own);
    wcscat_s(ini, FM5_PATH_CAPACITY, L"\\forzamastro.ini");
    show_pre_game = GetPrivateProfileIntW(L"startup", L"show_pre_game", 0, ini) != 0;
    wcscpy_s(runtime, FM5_PATH_CAPACITY, own);
    wcscat_s(runtime, FM5_PATH_CAPACITY, L"\\runtime");
    wcscpy_s(child, FM5_PATH_CAPACITY, runtime);
    wcscat_s(child, FM5_PATH_CAPACITY, L"\\ForzaMastro5.exe");
    wcscpy_s(ini, FM5_PATH_CAPACITY, runtime);
    wcscat_s(ini, FM5_PATH_CAPACITY, L"\\data\\RaceOneEngine.ini");
    if (!fm5_file_exists(child) || !fm5_file_exists(ini)) {
        result = fm5_fail(check_only, L"The game files are incomplete. Rebuild this game folder with the builder.", ERROR_FILE_NOT_FOUND);
        goto cleanup;
    }
    if (show_pre_game) {
        wcscpy_s(child, FM5_PATH_CAPACITY, runtime);
        wcscat_s(child, FM5_PATH_CAPACITY, L"\\ForzaMastro5.PreGame.exe");
        if (!fm5_file_exists(child)) {
            result = fm5_fail(check_only, L"The pre-game settings application is missing. Rebuild this game folder.", ERROR_FILE_NOT_FOUND);
            goto cleanup;
        }
    }
    if (check_only) { result = ERROR_SUCCESS; goto cleanup; }
    wcscpy_s(command, FM5_PATH_CAPACITY + 32, L"\"");
    wcscat_s(command, FM5_PATH_CAPACITY + 32, child);
    wcscat_s(command, FM5_PATH_CAPACITY + 32, show_pre_game ? L"\"" : L"\" --skip-launcher");
    environment = fm5_clean_environment();
    if (!environment) {
        result = fm5_fail(FALSE, L"Unable to prepare the game environment.", GetLastError());
        goto cleanup;
    }
    startup.cb = sizeof(startup);
    if (!CreateProcessW(child, command, NULL, NULL, FALSE, CREATE_UNICODE_ENVIRONMENT,
                        environment, runtime, &startup, &process)) {
        code = GetLastError();
        result = fm5_fail(FALSE, L"Unable to start the game. Check that its runtime files are complete.", code);
        goto cleanup;
    }
    CloseHandle(process.hThread);
    CloseHandle(process.hProcess);
    result = ERROR_SUCCESS;
cleanup:
    if (environment) HeapFree(GetProcessHeap(), 0, environment);
    if (command) HeapFree(GetProcessHeap(), 0, command);
    if (ini) HeapFree(GetProcessHeap(), 0, ini);
    if (child) HeapFree(GetProcessHeap(), 0, child);
    if (runtime) HeapFree(GetProcessHeap(), 0, runtime);
    if (own) HeapFree(GetProcessHeap(), 0, own);
    return result;
}
