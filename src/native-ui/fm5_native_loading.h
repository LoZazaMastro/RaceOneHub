#ifndef FM5_NATIVE_LOADING_H
#define FM5_NATIVE_LOADING_H
#include <windows.h>
#include <stdint.h>
/* This percentage is preparation-phase work only, never whole-game progress.
   FALSE means no measurable total: caller displays a phase label without %. */
BOOL FM5PreparationPercent(uint64_t completed,uint64_t total,unsigned *percent);
/* Animated activity only when the native phase exposes no total. */
BOOL FM5PreparationSweep(int width,uint64_t milliseconds,int *left,int *right);
HRESULT FM5PaintPreparationImage(HDC dc,const RECT *rect,HBITMAP background,
                                UINT width,UINT height,uint64_t completed,uint64_t total);
BOOL FM5BeginOriginalPreparation(void);
void FM5EndOriginalPreparation(void);
/* Redirect only original182a0 paint calls1b354/1c6ff to this helper. It preserves
   the genuine callback except while the calling thread owns warmup scope. */
__declspec(dllexport) void FM5PaintOriginalPreparation(HWND window,void *native_callback);
/* Typed native std::function callback ABI, guarded vtable29d970 method2. */
__declspec(dllexport) void FM5DrawPreparation(void *closure,HDC *dc,const RECT *rect);
/* Native completion-clear callback29d938 method2. Called only after preparation
   actually completed; retaining the image avoids a blank intermediate screen. */
__declspec(dllexport) void FM5DrawPreparationComplete(void *closure,HDC *dc,const RECT *rect);
#ifdef FM5_NATIVE_UI_TEST
void FM5TestComposeNativePainter(HWND window,void *native_callback,void(*native_paint)(HWND,void*));
#endif
HRESULT FM5RenderLoadingPreview(const wchar_t *asset,const wchar_t *output_bmp,
                               uint64_t completed,uint64_t total);
#endif
