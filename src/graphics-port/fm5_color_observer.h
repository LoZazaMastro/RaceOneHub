#pragma once
#include <windows.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Opt-in FM5_COLOR_DIAGNOSTICS=1. Call BEFORE UI Begin sets GPU state, from
   FM5BuildUiViewport entry. Reads only; no setters, drain, reset or GPU wait. */
void WINAPI FM5ObserveColorBeforeUi(void *ui);
/* Original bytecode is borrowed only during successful shader creation. */
void FM5ObservePixelShaderCreation(void *,const void *,SIZE_T);
void FM5ObservePostprocessDraw(void *,BOOL after);
void FM5ObservePostprocessMap(void *,void *,UINT,UINT,HRESULT,const void *);
void FM5ObservePostprocessUnmap(void *,void *,UINT);
/* Only the two guarded native Draw preparation call sites may call this. */
__declspec(dllexport) void *WINAPI FM5PrepareGraphicsObserved(void *,BYTE,void *);
#ifdef FM5_COLOR_OBSERVER_TEST
typedef void *(*FM5ColorUnwrap)(void *);
typedef LONGLONG (*FM5ColorClock)(void);
typedef void (*FM5ColorWrite)(const char *, DWORD);
typedef struct FM5ColorLock {void *mutex; BYTE held; BYTE reserved[7];} FM5ColorLock;
typedef FM5ColorLock *(*FM5ColorAcquire)(FM5ColorLock *,void *);
void FM5ColorObserverTestConfigure(const BYTE *, SIZE_T, void *, BOOL,
    FM5ColorUnwrap, FM5ColorClock, FM5ColorWrite, FM5ColorAcquire);
HRESULT FM5ColorObserverTestGuard(const BYTE *, SIZE_T);
UINT FM5ColorObserverTestClassify(const void *,SIZE_T);
typedef void * (WINAPI *FM5ColorPreparation)(void *,BYTE,void *);
void FM5ColorObserverTestPreparation(FM5ColorPreparation);
BOOL FM5ColorObserverTestViewMetadata(void *,void *,char *,UINT);
BOOL FM5ColorObserverTestActualBound(void *,void *,char *,UINT);
#endif
#ifdef __cplusplus
}
#endif
