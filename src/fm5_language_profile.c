#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <wchar.h>
#include "allocation-port/fm5_language.h"
#include "graphics-port/fm5_render_cadence.h"
#include "save-port/fm5_save_storage.h"

typedef struct FM5LanguageImage { BYTE *base; UINT size; } FM5LanguageImage;
HRESULT FM5ValidateNativeProfile(const FM5LanguageImage *, const void *);
void FM5ApplyPatches(const FM5LanguageImage *, const void *);
typedef void *(__cdecl *FM5SetNativeLocale)(const void *);
static FM5TextLanguagePlan prepared_languages[FM5_TEXT_LANGUAGE_COUNT];
static FM5SetNativeLocale set_native_locale;
static const BYTE *locale_host;
static SIZE_T locale_host_bytes;
static BOOL language_plans_ready;

/* The loader has finalized guest .text protection by the time main selects
   its locale. Temporarily open only the prepared instruction range; never
   leave the executable page writable after this late commit. */
static HRESULT commit_protected_language(const FM5TextLanguagePlan *plan) {
    DWORD previous = 0, ignored = 0;
    DWORD failure = ERROR_SUCCESS;
    if (!plan) return S_OK;
    if (!VirtualProtect(plan->site, sizeof(plan->replacement),
                        PAGE_EXECUTE_READWRITE, &previous))
        return HRESULT_FROM_WIN32(GetLastError());
    FM5CommitTextLanguage(plan);
    if (!FlushInstructionCache(GetCurrentProcess(), plan->site,
                               sizeof(plan->replacement)))
        failure = GetLastError();
    if (!VirtualProtect(plan->site, sizeof(plan->replacement), previous, &ignored)) {
        DWORD restore_failure = GetLastError();
        return HRESULT_FROM_WIN32(restore_failure);
    }
    return failure == ERROR_SUCCESS ? S_OK : HRESULT_FROM_WIN32(failure);
}

/* Compose host-selected text with the pure FM5 profile. All guards pass before
   either profile writes; language selects an existing title-owned prefix. */
__declspec(dllexport) void FM5ApplyPatchesWithLanguage(
    const FM5LanguageImage *image, const void *options) {
    FM5TextLanguagePlan plans[FM5_TEXT_LANGUAGE_COUNT] = {0};
    FM5RenderCadencePlan cadence = {0};
    FM5SaveStoragePlan *storage = NULL;
    HRESULT status = FM5ValidateNativeProfile(image, options);
    UINT target = SUCCEEDED(status) ? ((const UINT *)options)[3] : 0;
    const BYTE *host = (const BYTE *)GetModuleHandleW(NULL);
    SIZE_T host_bytes = 0;
    if (SUCCEEDED(status))
        status = FM5PrepareRenderCadence(image->base, image->size, target, &cadence);
    if (SUCCEEDED(status) && host) {
        const IMAGE_DOS_HEADER *dos = (const IMAGE_DOS_HEADER *)host;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew > 0 && dos->e_lfanew < 4096) {
            const IMAGE_NT_HEADERS64 *pe = (const IMAGE_NT_HEADERS64 *)(host + dos->e_lfanew);
            if (pe->Signature == IMAGE_NT_SIGNATURE &&
                pe->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC)
                host_bytes = pe->OptionalHeader.SizeOfImage;
        }
        for (UINT index = 0; index < FM5_TEXT_LANGUAGE_COUNT && SUCCEEDED(status); ++index) {
            const wchar_t *locale = FM5TextLanguageLocaleAt(index);
            status = FM5PrepareTextLanguageForLocale(host, host_bytes, image->base,
                image->size, locale, wcslen(locale), &plans[index]);
        }
    } else if (SUCCEEDED(status)) status = E_UNEXPECTED;
    if (SUCCEEDED(status)) status = FM5PrepareSaveStorage(host, host_bytes, &storage);
    if (SUCCEEDED(status)) {
        wchar_t probe[4];
        if (GetEnvironmentVariableW(L"FM5_SAVE_PATH_PROBE", probe, 4) == 1 && probe[0] == L'1') {
            status = FM5ProbePreparedSaveStorage(storage);
            fprintf(stderr, "{\"event\":\"fm5_native_save_path_probe\",\"hr\":\"0x%08lx\",\"directory_created\":false}\n",
                    (unsigned long)status);
        }
    }
    if (FAILED(status)) {
        FM5DiscardSaveStorage(&storage);
        FM5DiscardRenderCadence(&cadence);
        ULONG_PTR detail = (ULONG_PTR)(UINT)status;
        fprintf(stderr, "{\"event\":\"fm5_text_language_refused\",\"hr\":\"0x%08lx\"}\n", (unsigned long)status);
        RaiseException(0xe0f50031, EXCEPTION_NONCONTINUABLE, 1, &detail);
        return;
    }
    language_plans_ready = FALSE;
    FM5ApplyPatches(image, options);
    FM5CommitRenderCadence(&cadence);
    FM5CommitSaveStorage(&storage);
    fprintf(stderr, "{\"event\":\"fm5_render_cadence_profile\",\"target\":%u,\"alternate_frame_skip_removed\":%s}\n",
            target, target >= 60 ? "true" : "false");
    memcpy(prepared_languages, plans, sizeof(plans));
    locale_host = host; locale_host_bytes = host_bytes;
    set_native_locale = (FM5SetNativeLocale)(host + 0x3abf0);
    language_plans_ready = TRUE;
}

/* The native main sets its language AFTER profile application, before first
   guest entry. Preserve its exact std::wstring assignment and return value. */
__declspec(dllexport) void *FM5SetTextLanguageAfterProfile(const void *locale) {
    HRESULT status = E_UNEXPECTED;
    UINT selected_index = 0;
    void *result = NULL;
    const FM5TextLanguagePlan *selected = NULL;
    if (language_plans_ready && set_native_locale) {
        result = set_native_locale(locale);
        status = FM5ReadHostSelectedLocaleIndex(locale_host, locale_host_bytes, &selected_index);
        if (status == S_OK) {
            for (UINT index = 0; index < FM5_TEXT_LANGUAGE_COUNT; ++index)
                if (prepared_languages[index].language_index == selected_index) {
                    selected = &prepared_languages[index]; break;
                }
            if (!selected) status = E_UNEXPECTED;
        }
    }
    if (SUCCEEDED(status)) status = commit_protected_language(selected);
    if (FAILED(status)) {
        ULONG_PTR detail = (ULONG_PTR)(UINT)status;
        fprintf(stderr, "{\"event\":\"fm5_text_language_commit_refused\",\"hr\":\"0x%08lx\"}\n", (unsigned long)status);
        RaiseException(0xe0f50031, EXCEPTION_NONCONTINUABLE, 1, &detail);
        return result;
    }
    fprintf(stderr, "{\"event\":\"fm5_text_language_profile\",\"prefix\":\"%s\",\"enum\":%u,\"original_voice_fallback\":true}\n",
            selected ? selected->prefix : "auto", selected_index);
    return result;
}
