#pragma once
#include "fm5_d3d12_diagnostics.h"
#ifdef __cplusplus
extern "C" {
#endif
__declspec(dllexport) void WINAPI FM5DiagnosticExecuteCommandLists(void *queue,
    UINT count, void *const *lists, void *submission_owner, UINT64 sequence);
#ifdef __cplusplus
}
#endif
