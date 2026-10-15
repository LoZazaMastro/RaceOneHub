#pragma once
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct FM5TextLanguagePlan {
    BYTE *site;
    BYTE replacement[7];
    UINT language_index;
    char prefix[4];
} FM5TextLanguagePlan;
#define FM5_TEXT_LANGUAGE_COUNT 16
/* Ordinal0..15 enumerates supported canonical locale strings; NULL otherwise. */
const wchar_t *FM5TextLanguageLocaleAt(UINT ordinal);
/* Explicit preparation is valid before the host assigns its global locale. */
HRESULT FM5PrepareTextLanguageForLocale(const BYTE *host,SIZE_T host_bytes,
    BYTE *guest,SIZE_T guest_bytes,const wchar_t *locale,SIZE_T length,
    FM5TextLanguagePlan *plan);
/* Host-only validation: S_FALSE=System, S_OK=actual FM5 enum0/2..16.
   Explicit en-US returns S_OK with index0, distinct from empty System. */
HRESULT FM5ReadHostSelectedLocaleIndex(const BYTE *host,SIZE_T host_bytes,UINT *language_index);
/* Prepare is read-only. Call before every other profile mutation. */
HRESULT FM5PrepareTextLanguage(const BYTE *host, SIZE_T host_bytes,
    BYTE *guest, SIZE_T guest_bytes, FM5TextLanguagePlan *plan);
/* Commit only a successful prepared plan after all profile guards have passed. */
void FM5CommitTextLanguage(const FM5TextLanguagePlan *plan);
#ifdef __cplusplus
}
#endif
