#define WIN32_LEAN_AND_MEAN
#include "fm5_scene_files.h"
#include <stdint.h>
#include <wchar.h>
#include <stdio.h>
#include <string.h>
#include <wctype.h>

static volatile LONG scene_rate;
static const wchar_t *names[] = {L"FrontEnd.xml", L"MiniAutoVista.xml", L"PhotoMode.xml",
    L"PostRace.xml", L"PreRace_321.xml", L"Replay.xml"};
typedef struct Paths { wchar_t input[32768], source[32768], local[32768]; } Paths;

__declspec(dllexport) void FM5SetSceneFileRate(UINT target) {
    InterlockedExchange(&scene_rate, (LONG)target);
}

static int module_folder(wchar_t *path) {
    HMODULE module = NULL;
    DWORD length;
    wchar_t *slash;
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT, (LPCWSTR)(uintptr_t)&module_folder, &module)) return 0;
    length = GetModuleFileNameW(module, path, 32768);
    if (!length || length >= 32768 || !(slash = wcsrchr(path, L'\\'))) return 0;
    slash[1] = 0;
    return 1;
}

static int full_path(const wchar_t *input, wchar_t *output) {
    DWORD n = GetFullPathNameW(input, 32768, output, NULL);
    if (!n || n >= 32768) return 0;
    for (DWORD i = 0; i < n; ++i) if (output[i] == L'/') output[i] = L'\\';
    /* Canonical comparisons treat ordinary extended drive paths identically. */
    if (!wcsncmp(output, L"\\\\?\\", 4) && output[5] == L':')
        memmove(output, output + 4, (wcslen(output + 4) + 1) * sizeof(wchar_t));
    return 1;
}

/* The engine writes UTF-8 INI files; Win32 profile APIs otherwise assume the
   legacy system codepage. Decode explicitly so non-ASCII game paths work. */
static int read_source_folder(const wchar_t *ini, wchar_t *output) {
    HANDLE file = CreateFileW(ini, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    LARGE_INTEGER size;
    DWORD read = 0;
    char *raw = NULL;
    wchar_t *text = NULL;
    int result = 0, launcher = 0, count;
    if (file == INVALID_HANDLE_VALUE) return 0;
    if (!GetFileSizeEx(file, &size) || size.QuadPart <= 0 || size.QuadPart > 1024 * 1024) goto done;
    raw = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, (SIZE_T)size.QuadPart + 2);
    text = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ((SIZE_T)size.QuadPart + 1) * sizeof(wchar_t));
    if (!raw || !text || !ReadFile(file, raw, (DWORD)size.QuadPart, &read, NULL) || read != size.QuadPart) goto done;
    if (read >= 2 && (unsigned char)raw[0] == 0xff && (unsigned char)raw[1] == 0xfe) {
        if (read % 2) goto done;
        memcpy(text, raw + 2, read - 2);
    } else {
        DWORD bom = read >= 3 && !memcmp(raw, "\xef\xbb\xbf", 3) ? 3 : 0;
        count = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, raw + bom, (int)(read - bom), text, (int)read);
        if (!count) goto done;
        text[count] = 0;
    }
    for (wchar_t *line = text, *next; line && *line; line = next) {
        wchar_t *end, *equal, *value;
        next = wcschr(line, L'\n'); if (next) *next++ = 0;
        while (iswspace(*line)) ++line;
        end = line + wcslen(line);
        while (end > line && iswspace(end[-1])) *--end = 0;
        if (*line == L'[') { launcher = !_wcsicmp(line, L"[launcher]"); continue; }
        if (!launcher || !(equal = wcschr(line, L'='))) continue;
        *equal = 0; end = equal;
        while (end > line && iswspace(end[-1])) *--end = 0;
        if (_wcsicmp(line, L"game_folder")) continue;
        value = equal + 1; while (iswspace(*value)) ++value;
        if (*value && !wcscpy_s(output, 32768, value)) result = 1;
        break;
    }
done:
    if (raw) HeapFree(GetProcessHeap(), 0, raw);
    if (text) HeapFree(GetProcessHeap(), 0, text);
    CloseHandle(file);
    return result;
}

/* Only six exact, read-only source paths are redirected. The selected game
   directory remains the actual VFS root; all other file access is original.
   A missing override is an error, never a silent return to the 30 FPS recipe. */
__declspec(dllexport) HANDLE WINAPI FM5CreateGameFileW(LPCWSTR name, DWORD access, DWORD sharing,
    LPSECURITY_ATTRIBUTES security, DWORD creation, DWORD flags, HANDLE template_file) {
    const wchar_t *leaf;
    UINT index;
    Paths *paths;
    DWORD error;
    HANDLE result;
    LONG rate = InterlockedCompareExchange(&scene_rate, 0, 0);
    if (!name || rate < 60 || creation != OPEN_EXISTING ||
        !(access & (GENERIC_READ | FILE_READ_DATA)) ||
        (access & (GENERIC_WRITE | GENERIC_ALL | FILE_WRITE_DATA | FILE_APPEND_DATA |
                   FILE_WRITE_EA | FILE_WRITE_ATTRIBUTES | DELETE | WRITE_DAC | WRITE_OWNER)) ||
        (flags & FILE_FLAG_DELETE_ON_CLOSE))
        return CreateFileW(name, access, sharing, security, creation, flags, template_file);
    leaf = name;
    for (const wchar_t *p = name; *p; ++p) if (*p == L'\\' || *p == L'/') leaf = p + 1;
    for (index = 0; index < ARRAYSIZE(names); ++index) if (!_wcsicmp(leaf, names[index])) break;
    if (index == ARRAYSIZE(names))
        return CreateFileW(name, access, sharing, security, creation, flags, template_file);
    paths = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(*paths));
    if (!paths) { SetLastError(ERROR_NOT_ENOUGH_MEMORY); return INVALID_HANDLE_VALUE; }
    if (!full_path(name, paths->input) || !module_folder(paths->local) ||
        wcscat_s(paths->local, 32768, L"data\\RaceOneEngine.ini") ||
        !read_source_folder(paths->local, paths->source) ||
        swprintf_s(paths->local, 32768, L"%ls\\Mount\\media\\renderscenarios\\%ls", paths->source, names[index]) < 0 ||
        !full_path(paths->local, paths->source) || CompareStringOrdinal(paths->source, -1, paths->input, -1, TRUE) != CSTR_EQUAL) {
        HeapFree(GetProcessHeap(), 0, paths);
        return CreateFileW(name, access, sharing, security, creation, flags, template_file);
    }
    /* Do not conceal removal of the user's source file behind an old override. */
    if (GetFileAttributesW(name) == INVALID_FILE_ATTRIBUTES) {
        error = GetLastError(); HeapFree(GetProcessHeap(), 0, paths); SetLastError(error);
        return INVALID_HANDLE_VALUE;
    }
    if (!module_folder(paths->local) || wcscat_s(paths->local, 32768, L"data\\scene-overrides\\") ||
        wcscat_s(paths->local, 32768, names[index])) {
        HeapFree(GetProcessHeap(), 0, paths); SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return INVALID_HANDLE_VALUE;
    }
    result = CreateFileW(paths->local, access, sharing, security, creation, flags, template_file);
    error = GetLastError();
    HeapFree(GetProcessHeap(), 0, paths); SetLastError(error);
    return result;
}
