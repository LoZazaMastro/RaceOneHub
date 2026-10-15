#pragma once
#include <windows.h>
#include <stddef.h>

typedef struct FM5SaveStoragePlan FM5SaveStoragePlan;
/* Prepare allocates a genuine host wstring, resolves Saved Games, validates
   pinned helpers, and creates no directory. Commit consumes the prepared plan.
   Call before guest entry on the exclusively owned startup thread. */
HRESULT FM5PrepareSaveStorage(const BYTE *host, SIZE_T size, FM5SaveStoragePlan **out);
/* Diagnostics only: genuine path helper/string lifecycle, exact result check.
   Does not create directories or instantiate a storage provider. */
HRESULT FM5ProbePreparedSaveStorage(const FM5SaveStoragePlan *plan);
void FM5CommitSaveStorage(FM5SaveStoragePlan **plan);
void FM5DiscardSaveStorage(FM5SaveStoragePlan **plan);

/* Replaces ONLY the CALL at original Az RVA1870cb. Same three-argument ABI as
   Az81dc0: (uninitialized output wstring, executable-root wstring, title ID).
   Returns an owned genuine host wstring; original caller destroys it. */
__declspec(dllexport) void *WINAPI FM5BuildPersistentSaveTitlePath(
    void *output, const void *original_root, const void *title_id);

#ifdef FM5_SAVE_STORAGE_TEST
typedef void *(WINAPI *FM5SaveAssign)(void *, const WCHAR *, SIZE_T);
typedef void (WINAPI *FM5SaveDestroy)(void *);
typedef void *(WINAPI *FM5SaveBuildTitle)(void *, const void *, const void *);
void FM5SaveStorageTestBackends(FM5SaveAssign, FM5SaveDestroy, FM5SaveBuildTitle);
HRESULT FM5SaveStorageTestPrepare(const BYTE *, SIZE_T, const WCHAR *, FM5SaveStoragePlan **);
void FM5SaveStorageTestReset(void);
HRESULT FM5SaveStorageTestGuard(const BYTE *, SIZE_T);
#endif
