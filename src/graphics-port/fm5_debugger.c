#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <dbghelp.h>
#pragma comment(lib, "dbghelp.lib")

#if !defined(_M_X64)
#error This debugger requires AMD64.
#endif

static FILE *output;
static DWORD child_pid;
static unsigned long sequence;
static unsigned long cpp_exceptions;
static DWORD break_rva;
static void *break_address;
static unsigned char break_original;
static int break_active;
static void begin_event(const char *name);
static void end_event(void);

static void capture_unwind(HANDLE process, HANDLE thread, const CONTEXT *original) {
    STACKFRAME64 frame;
    CONTEXT context = *original;
    DWORD n;
    SymSetOptions(SYMOPT_DEFERRED_LOADS | SYMOPT_FAIL_CRITICAL_ERRORS |
                  SYMOPT_NO_PROMPTS | SYMOPT_IGNORE_NT_SYMPATH);
    if (!SymInitialize(process, "", TRUE)) return;
    ZeroMemory(&frame, sizeof(frame));
    frame.AddrPC.Offset = context.Rip;
    frame.AddrPC.Mode = AddrModeFlat;
    frame.AddrFrame.Offset = context.Rbp;
    frame.AddrFrame.Mode = AddrModeFlat;
    frame.AddrStack.Offset = context.Rsp;
    frame.AddrStack.Mode = AddrModeFlat;
    for (n = 0; n < 48; ++n) {
        if (!StackWalk64(IMAGE_FILE_MACHINE_AMD64, process, thread, &frame, &context,
                        NULL, SymFunctionTableAccess64, SymGetModuleBase64, NULL)) break;
        if (!frame.AddrPC.Offset) break;
        begin_event("unwind_frame");
        fprintf(output, ",\"depth\":%lu,\"pc\":\"0x%llx\","
                "\"sp\":\"0x%llx\",\"module_base\":\"0x%llx\"",
                n, frame.AddrPC.Offset, frame.AddrStack.Offset,
                SymGetModuleBase64(process, frame.AddrPC.Offset));
        end_event();
    }
    fflush(output);
    SymCleanup(process);
}

static void json_string(const char *value) {
    const unsigned char *p = (const unsigned char *)value;
    fputc('"', output);
    while (*p) {
        unsigned char c = *p++;
        if (c == '"' || c == '\\') fprintf(output, "\\%c", c);
        else if (c < 0x20) fprintf(output, "\\u%04x", c);
        else fputc(c, output);
    }
    fputc('"', output);
}

static void json_wstring(const wchar_t *value) {
    char encoded[32768];
    int count = WideCharToMultiByte(CP_UTF8, 0, value, -1, encoded,
                                    sizeof(encoded), NULL, NULL);
    if (!count) encoded[0] = 0;
    json_string(encoded);
}

static void begin_event(const char *name) {
    FILETIME time;
    ULARGE_INTEGER ticks;
    GetSystemTimeAsFileTime(&time);
    ticks.LowPart = time.dwLowDateTime;
    ticks.HighPart = time.dwHighDateTime;
    fprintf(output, "{\"seq\":%lu,\"utc_ms\":%llu,\"pid\":%lu,\"event\":",
        ++sequence, (ticks.QuadPart - 116444736000000000ULL) / 10000ULL, child_pid);
    json_string(name);
}

static void end_event(void) {
    fputs("}\n", output);
    fflush(output);
}

static void log_error(const char *operation, DWORD code) {
    begin_event("error");
    fputs(",\"operation\":", output);
    json_string(operation);
    fprintf(output, ",\"win32_error\":%lu", code);
    end_event();
}

static int canonical_path(const wchar_t *path, wchar_t *result, DWORD capacity) {
    wchar_t buffer[32768];
    const wchar_t *resolved;
    DWORD length;
    HANDLE file = CreateFileW(path, FILE_READ_ATTRIBUTES,
        FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, NULL,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;
    length = GetFinalPathNameByHandleW(file, buffer, _countof(buffer), FILE_NAME_NORMALIZED);
    CloseHandle(file);
    if (!length || length >= _countof(buffer)) return 0;
    resolved = wcsncmp(buffer, L"\\\\?\\", 4) == 0 ? buffer + 4 : buffer;
    return wcscpy_s(result, capacity, resolved) == 0;
}

static int allowed_target(const wchar_t *path, int fixture) {
    wchar_t self[32768], expected[32768], actual[32768];
    wchar_t *separator;
    int i;
    DWORD length = GetModuleFileNameW(NULL, self, _countof(self));
    if (!length || length >= _countof(self)) return 0;
    for (i = 0; i < 3; ++i) {
        separator = wcsrchr(self, L'\\');
        if (!separator) return 0;
        *separator = 0;
    }
    if (_snwprintf_s(expected, _countof(expected), _TRUNCATE, L"%s%s", self,
        fixture ? L"\\analysis\\graphics-port\\debug_fixture.exe" :
                  L"\\runtime\\pc\\ForzaMastro5.exe") < 0) return 0;
    if (!canonical_path(path, actual, _countof(actual))) return 0;
    if (!canonical_path(expected, expected, _countof(expected))) return 0;
    return _wcsicmp(actual, expected) == 0;
}

static int append_char(wchar_t *buffer, size_t capacity, size_t *used, wchar_t c) {
    if (*used + 1 >= capacity) return 0;
    buffer[(*used)++] = c;
    buffer[*used] = 0;
    return 1;
}

static int append_argument(wchar_t *buffer, size_t capacity, size_t *used,
                           const wchar_t *argument) {
    size_t slashes;
    if (*used && !append_char(buffer, capacity, used, L' ')) return 0;
    if (!append_char(buffer, capacity, used, L'"')) return 0;
    while (*argument) {
        slashes = 0;
        while (*argument == L'\\') { ++slashes; ++argument; }
        if (*argument == L'"' || !*argument) slashes *= 2;
        while (slashes--) if (!append_char(buffer, capacity, used, L'\\')) return 0;
        if (*argument == L'"' && !append_char(buffer, capacity, used, L'\\')) return 0;
        if (*argument && !append_char(buffer, capacity, used, *argument++)) return 0;
    }
    return append_char(buffer, capacity, used, L'"');
}

static void log_module(const char *event, DWORD thread_id, void *base, HANDLE file) {
    wchar_t path[32768] = L"";
    DWORD count;
    if (file && file != INVALID_HANDLE_VALUE) {
        count = GetFinalPathNameByHandleW(file, path, _countof(path), FILE_NAME_NORMALIZED);
        if (!count || count >= _countof(path)) path[0] = 0;
    }
    begin_event(event);
    fprintf(output, ",\"tid\":%lu,\"base\":\"0x%llx\",\"path\":",
        thread_id, (unsigned long long)(ULONG_PTR)base);
    json_wstring(path);
    end_event();
}

static void capture_memory(HANDLE process, const char *event, const void *address,
                           SIZE_T capacity) {
    unsigned char buffer[1024];
    SIZE_T read = 0, i;
    BOOL ok;
    if (capacity > sizeof(buffer)) capacity = sizeof(buffer);
    ok = ReadProcessMemory(process, address, buffer, capacity, &read);
    DWORD error = ok ? ERROR_SUCCESS : GetLastError();
    begin_event(event);
    fprintf(output, ",\"address\":\"0x%llx\",\"requested\":%llu,\"read\":%llu,"
        "\"win32_error\":%lu,\"hex\":\"", (unsigned long long)(ULONG_PTR)address,
        (unsigned long long)capacity, (unsigned long long)read, error);
    for (i = 0; i < read; ++i) fprintf(output, "%02x", buffer[i]);
    fputc('"', output);
    if (strstr(event, "stack_bytes")) {
        fputs(",\"qwords\":[", output);
        for (i = 0; i + 8 <= read; i += 8) {
            SIZE_T j;
            unsigned long long word = 0;
            for (j = 0; j < 8; ++j) word |= (unsigned long long)buffer[i + j] << (j * 8);
            fprintf(output, "%s\"0x%llx\"", i ? "," : "", word);
        }
        fputc(']', output);
    }
    end_event();
}

static void capture_context(HANDLE process, DWORD thread_id, int breakpoint) {
    CONTEXT context;
    HANDLE thread = OpenThread(THREAD_GET_CONTEXT | THREAD_QUERY_LIMITED_INFORMATION,
                                FALSE, thread_id);
    if (!thread) { log_error("OpenThread", GetLastError()); return; }
    if (GetProcessIdOfThread(thread) != child_pid) {
        log_error("thread_ownership", ERROR_ACCESS_DENIED);
        CloseHandle(thread);
        return;
    }
    ZeroMemory(&context, sizeof(context));
    context.ContextFlags = CONTEXT_FULL;
    if (!GetThreadContext(thread, &context)) {
        log_error("GetThreadContext", GetLastError());
        CloseHandle(thread);
        return;
    }
    begin_event(breakpoint ? "breakpoint_context_amd64" : "crash_context_amd64");
    fprintf(output, ",\"tid\":%lu,\"rip\":\"0x%llx\",\"rsp\":\"0x%llx\","
        "\"rbp\":\"0x%llx\",\"rax\":\"0x%llx\",\"rbx\":\"0x%llx\","
        "\"rcx\":\"0x%llx\",\"rdx\":\"0x%llx\",\"rsi\":\"0x%llx\","
        "\"rdi\":\"0x%llx\",\"r8\":\"0x%llx\",\"r9\":\"0x%llx\","
        "\"r10\":\"0x%llx\",\"r11\":\"0x%llx\",\"r12\":\"0x%llx\","
        "\"r13\":\"0x%llx\",\"r14\":\"0x%llx\",\"r15\":\"0x%llx\",\"eflags\":%lu",
        thread_id, context.Rip, context.Rsp, context.Rbp, context.Rax, context.Rbx,
        context.Rcx, context.Rdx, context.Rsi, context.Rdi, context.R8, context.R9,
        context.R10, context.R11, context.R12, context.R13, context.R14, context.R15,
        context.EFlags);
    end_event();
    capture_memory(process, breakpoint ? "breakpoint_stack_bytes" : "crash_stack_bytes",
        (void *)(ULONG_PTR)context.Rsp, breakpoint ? 1024 : 128);
    if (!breakpoint)
        capture_memory(process, "crash_extended_stack_bytes", (void *)(ULONG_PTR)context.Rsp, 1024);
    capture_memory(process, breakpoint ? "breakpoint_instruction_bytes" : "crash_instruction_bytes",
        (void *)(ULONG_PTR)context.Rip, 64);
    if (breakpoint) {
        capture_memory(process, "breakpoint_rcx_memory", (void *)(ULONG_PTR)context.Rcx, 64);
        capture_memory(process, "breakpoint_rdx_memory", (void *)(ULONG_PTR)context.Rdx, 256);
        capture_memory(process, "breakpoint_r8_memory", (void *)(ULONG_PTR)context.R8, 64);
        capture_memory(process, "breakpoint_r9_memory", (void *)(ULONG_PTR)context.R9, 32);
        if (break_rva == 0x18fc18) {
            ULONG_PTR range[2] = {0, 0};
            SIZE_T read = 0;
            if (ReadProcessMemory(process, (void *)(ULONG_PTR)(context.Rdx + 0x48),
                                  range, sizeof(range), &read) && read == sizeof(range) &&
                range[0] && range[1] >= range[0] && range[1] - range[0] <= 65536) {
                SIZE_T offset, length = range[1] - range[0];
                for (offset = 0; offset < length; offset += 1024)
                    capture_memory(process, "shape_graph_bytes", (void *)(range[0] + offset),
                        length - offset < 1024 ? length - offset : 1024);
            }
        }
    }
    if (!breakpoint) capture_unwind(process, thread, &context);
    CloseHandle(thread);
}

static int write_break_byte(HANDLE process, unsigned char value) {
    DWORD old_protection, ignored;
    SIZE_T written = 0;
    BOOL ok;
    if (!VirtualProtectEx(process, break_address, 1, PAGE_EXECUTE_READWRITE, &old_protection)) return 0;
    ok = WriteProcessMemory(process, break_address, &value, 1, &written);
    if (!VirtualProtectEx(process, break_address, 1, old_protection, &ignored)) return 0;
    if (!ok || written != 1) return 0;
    return FlushInstructionCache(process, break_address, 1) != FALSE;
}

static int install_breakpoint(HANDLE process, void *base) {
    IMAGE_DOS_HEADER dos;
    IMAGE_NT_HEADERS64 nt;
    IMAGE_SECTION_HEADER section;
    SIZE_T read;
    DWORD index;
    ULONG_PTR section_table;
    if (!ReadProcessMemory(process, base, &dos, sizeof(dos), &read) || read != sizeof(dos) ||
        dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew <= 0 || dos.e_lfanew > 0x100000) return 0;
    if (!ReadProcessMemory(process, (char *)base + dos.e_lfanew, &nt, sizeof(nt), &read) ||
        read != sizeof(nt) || nt.Signature != IMAGE_NT_SIGNATURE ||
        nt.FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        !nt.FileHeader.NumberOfSections || nt.FileHeader.NumberOfSections > 96 ||
        break_rva >= nt.OptionalHeader.SizeOfImage) return 0;
    section_table = (ULONG_PTR)base + dos.e_lfanew + FIELD_OFFSET(IMAGE_NT_HEADERS64, OptionalHeader) +
                    nt.FileHeader.SizeOfOptionalHeader;
    for (index = 0; index < nt.FileHeader.NumberOfSections; ++index) {
        if (!ReadProcessMemory(process, (void *)(section_table + index*sizeof(section)),
            &section, sizeof(section), &read) || read != sizeof(section)) return 0;
        if (break_rva < section.VirtualAddress ||
            break_rva - section.VirtualAddress >= section.Misc.VirtualSize) continue;
        if (!(section.Characteristics & IMAGE_SCN_MEM_EXECUTE)) return 0;
        break_address = (char *)base + break_rva;
        if (!ReadProcessMemory(process, break_address, &break_original, 1, &read) || read != 1 ||
            break_original == 0xcc || !write_break_byte(process, 0xcc)) return 0;
        break_active = 1;
        begin_event("breakpoint_installed");
        fprintf(output, ",\"rva\":\"0x%lx\",\"address\":\"0x%llx\",\"original_byte\":%u",
            break_rva, (unsigned long long)(ULONG_PTR)break_address, break_original);
        end_event();
        return 1;
    }
    return 0;
}

static int handle_breakpoint(HANDLE process, DWORD thread_id) {
    CONTEXT context;
    HANDLE thread;
    if (!write_break_byte(process, break_original)) return 0;
    break_active = 0;
    thread = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_QUERY_LIMITED_INFORMATION,
        FALSE, thread_id);
    if (!thread) return 0;
    ZeroMemory(&context, sizeof(context));
    context.ContextFlags = CONTEXT_CONTROL;
    if (GetProcessIdOfThread(thread) != child_pid || !GetThreadContext(thread, &context)) {
        CloseHandle(thread); return 0;
    }
    if (context.Rip != (ULONG_PTR)break_address + 1) { CloseHandle(thread); return 0; }
    context.Rip = (ULONG_PTR)break_address;
    if (!SetThreadContext(thread, &context)) { CloseHandle(thread); return 0; }
    CloseHandle(thread);
    begin_event("breakpoint_hit");
    fprintf(output, ",\"tid\":%lu,\"original_instruction_restored\":true", thread_id);
    end_event();
    capture_context(process, thread_id, 1);
    return 1;
}

static BOOL CALLBACK close_child_window(HWND window, LPARAM unused) {
    DWORD pid = 0;
    (void)unused;
    GetWindowThreadProcessId(window, &pid);
    if (pid == child_pid) PostMessageW(window, WM_CLOSE, 0, 0);
    return TRUE;
}

static void usage(void) {
    fwprintf(stderr, L"Usage: fm5_debugger.exe --log FILE [--timeout SECONDS] [--break-rva HEX] [--fixture] -- EXE [ARGS...]\n");
}

int wmain(int argc, wchar_t **argv) {
    STARTUPINFOW startup;
    PROCESS_INFORMATION process;
    DEBUG_EVENT event;
    wchar_t command[32768] = L"", directory[32768], target[32768];
    const wchar_t *log_path = NULL;
    wchar_t *separator;
    size_t used = 0;
    DWORD timeout_seconds = 30, exit_code = 1;
    ULONGLONG started, close_requested = 0;
    int fixture = 0, first_breakpoint = 1, exited = 0, terminated = 0;
    int index = 1, target_index, result = 0;
    while (index < argc && wcscmp(argv[index], L"--")) {
        if (!wcscmp(argv[index], L"--log") && index + 1 < argc) log_path = argv[++index];
        else if (!wcscmp(argv[index], L"--timeout") && index + 1 < argc) {
            wchar_t *end;
            timeout_seconds = wcstoul(argv[++index], &end, 10);
            if (*end || !timeout_seconds || timeout_seconds > 600) { usage(); return 2; }
        } else if (!wcscmp(argv[index], L"--break-rva") && index + 1 < argc) {
            wchar_t *end;
            break_rva = wcstoul(argv[++index], &end, 0);
            if (*end || !break_rva || break_rva > 0x7fffffff) { usage(); return 2; }
        } else if (!wcscmp(argv[index], L"--fixture")) fixture = 1;
        else { usage(); return 2; }
        ++index;
    }
    target_index = ++index;
    if (!log_path || target_index >= argc || !allowed_target(argv[target_index], fixture)) {
        fwprintf(stderr, L"Refused: target must be this workspace's ForzaMastro5.exe or the explicit test fixture.\n");
        return 2;
    }
    if (_wfopen_s(&output, log_path, L"wb") || !output) return 2;
    if (!canonical_path(argv[target_index], target, _countof(target))) {
        log_error("canonical_target", GetLastError()); result = 2; goto done;
    }
    wcscpy_s(directory, _countof(directory), target);
    separator = wcsrchr(directory, L'\\');
    if (!separator) { result = 2; goto done; }
    *separator = 0;
    for (index = target_index; index < argc; ++index) {
        if (!append_argument(command, _countof(command), &used, argv[index])) {
            log_error("command_line_size", ERROR_BUFFER_OVERFLOW); result = 2; goto done;
        }
    }
    ZeroMemory(&startup, sizeof(startup));
    ZeroMemory(&process, sizeof(process));
    startup.cb = sizeof(startup);
    if (!CreateProcessW(target, command, NULL, NULL, FALSE,
        DEBUG_ONLY_THIS_PROCESS | CREATE_NEW_PROCESS_GROUP, NULL, directory, &startup, &process)) {
        log_error("CreateProcessW", GetLastError()); result = 2; goto done;
    }
    child_pid = process.dwProcessId;
    CloseHandle(process.hThread);
    DebugSetProcessKillOnExit(TRUE);
    started = GetTickCount64();
    begin_event("session_start");
    fputs(",\"target\":", output); json_wstring(target);
    fprintf(output, ",\"timeout_seconds\":%lu,\"debug_only_child\":true", timeout_seconds);
    end_event();
    while (!exited) {
        DWORD continuation = DBG_CONTINUE;
        ULONGLONG now = GetTickCount64();
        if (!close_requested && now - started >= timeout_seconds * 1000ULL) {
            close_requested = now;
            begin_event("timeout_close_requested"); end_event();
            EnumWindows(close_child_window, 0);
        } else if (close_requested && !terminated && now - close_requested >= 3000) {
            terminated = 1;
            begin_event("timeout_terminate_child"); end_event();
            if (!TerminateProcess(process.hProcess, 0xE0000001))
                log_error("TerminateProcess_child", GetLastError());
        }
        if (!WaitForDebugEventEx(&event, 100)) {
            DWORD error = GetLastError();
            if (error == ERROR_SEM_TIMEOUT) continue;
            log_error("WaitForDebugEventEx", error);
            TerminateProcess(process.hProcess, 0xE0000002);
            result = 2;
            break;
        }
        if (event.dwProcessId != child_pid) {
            log_error("debug_event_ownership", ERROR_ACCESS_DENIED);
            continuation = DBG_EXCEPTION_NOT_HANDLED;
        } else switch (event.dwDebugEventCode) {
        case CREATE_PROCESS_DEBUG_EVENT:
            log_module("process_image", event.dwThreadId,
                event.u.CreateProcessInfo.lpBaseOfImage, event.u.CreateProcessInfo.hFile);
            if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            if (break_rva && !install_breakpoint(process.hProcess, event.u.CreateProcessInfo.lpBaseOfImage)) {
                log_error("install_breakpoint", GetLastError());
                TerminateProcess(process.hProcess, 0xE0000004);
                result = 2;
            }
            break;
        case LOAD_DLL_DEBUG_EVENT:
            log_module("module_load", event.dwThreadId,
                event.u.LoadDll.lpBaseOfDll, event.u.LoadDll.hFile);
            if (event.u.LoadDll.hFile) CloseHandle(event.u.LoadDll.hFile);
            break;
        case CREATE_THREAD_DEBUG_EVENT:
            begin_event("thread_start");
            fprintf(output, ",\"tid\":%lu,\"start_address\":\"0x%llx\"", event.dwThreadId,
                (unsigned long long)(ULONG_PTR)event.u.CreateThread.lpStartAddress);
            end_event();
            break;
        case EXIT_THREAD_DEBUG_EVENT:
            begin_event("thread_exit");
            fprintf(output, ",\"tid\":%lu,\"exit_code\":\"0x%08lx\"", event.dwThreadId,
                event.u.ExitThread.dwExitCode);
            end_event();
            break;
        case UNLOAD_DLL_DEBUG_EVENT:
            log_module("module_unload", event.dwThreadId, event.u.UnloadDll.lpBaseOfDll, NULL);
            break;
        case EXCEPTION_DEBUG_EVENT: {
            const EXCEPTION_DEBUG_INFO *info = &event.u.Exception;
            const EXCEPTION_RECORD *record = &info->ExceptionRecord;
            DWORD n;
            continuation = DBG_EXCEPTION_NOT_HANDLED;
            if (record->ExceptionCode == EXCEPTION_BREAKPOINT && info->dwFirstChance &&
                break_active && record->ExceptionAddress == break_address) {
                if (!handle_breakpoint(process.hProcess, event.dwThreadId)) {
                    log_error("restore_breakpoint", GetLastError());
                    TerminateProcess(process.hProcess, 0xE0000005);
                    result = 2;
                }
                continuation = DBG_CONTINUE;
                break;
            }
            if (record->ExceptionCode == EXCEPTION_BREAKPOINT && info->dwFirstChance && first_breakpoint) {
                first_breakpoint = 0;
                continuation = DBG_CONTINUE;
                begin_event("initial_debug_breakpoint"); end_event();
                break;
            }
            if (record->ExceptionCode == 0xE06D7363 && info->dwFirstChance) {
                ++cpp_exceptions;
                break;
            }
            begin_event("exception");
            fprintf(output, ",\"tid\":%lu,\"first_chance\":%s,\"code\":\"0x%08lx\","
                "\"address\":\"0x%llx\",\"parameters\":[", event.dwThreadId,
                info->dwFirstChance ? "true" : "false", record->ExceptionCode,
                (unsigned long long)(ULONG_PTR)record->ExceptionAddress);
            for (n = 0; n < record->NumberParameters && n < EXCEPTION_MAXIMUM_PARAMETERS; ++n)
                fprintf(output, "%s\"0x%llx\"", n ? "," : "", (unsigned long long)record->ExceptionInformation[n]);
            fputc(']', output);
            end_event();
            if (record->ExceptionCode == 0xe05f0055 && record->NumberParameters && info->dwFirstChance)
                capture_memory(process.hProcess, "fatal_message_storage",
                    (void *)(ULONG_PTR)record->ExceptionInformation[0], 512);
            if (!info->dwFirstChance) capture_context(process.hProcess, event.dwThreadId, 0);
            break;
        }
        case EXIT_PROCESS_DEBUG_EVENT:
            exit_code = event.u.ExitProcess.dwExitCode;
            begin_event("process_exit");
            fprintf(output, ",\"exit_code\":\"0x%08lx\",\"suppressed_cpp_first_chance\":%lu",
                exit_code, cpp_exceptions);
            end_event();
            exited = 1;
            break;
        default:
            break;
        }
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, continuation)) {
            log_error("ContinueDebugEvent", GetLastError());
            TerminateProcess(process.hProcess, 0xE0000003);
            result = 2;
            break;
        }
    }
    CloseHandle(process.hProcess);
    if (!result && exited) result = (int)exit_code;
done:
    fclose(output);
    return result;
}
