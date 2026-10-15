#ifndef FM5_NATIVE_UI_H
#define FM5_NATIVE_UI_H
#include <windows.h>
#include <stdint.h>
#ifdef __cplusplus
extern "C" {
#endif
#define FM5_UI_MAX_CHOICES 20
#define FM5_UI_MAX_ROWS 12
typedef struct FM5UIChoice { const wchar_t *label; unsigned value; } FM5UIChoice;
typedef struct FM5UIRow {
    const wchar_t *label;
    FM5UIChoice choices[FM5_UI_MAX_CHOICES];
    unsigned count, selected;
    BOOL enabled;
} FM5UIRow;
typedef struct FM5UIModel {
    FM5UIRow rows[FM5_UI_MAX_ROWS];
    unsigned count, focused;
    /* Called on Start. A failed commit keeps the menu open and displays error. */
    HRESULT (*commit)(struct FM5UIModel *, void *context);
    /* Genuine asynchronous preparation snapshot. FALSE means work is still
       pending; callers must keep pumping HWND messages rather than sleep. */
    BOOL (*preparation)(void *context,uint64_t *completed,uint64_t *total);
    void *context;
    wchar_t error[192];
} FM5UIModel;
/* Adopts neither HWND nor model. Must run on the existing HWND owner thread.
   Returns S_OK only after successful commit, S_FALSE on Quit, failure on setup.
   Never creates another window, starts another process or destroys the HWND. */
HRESULT FM5RunNativePreGame(HWND window, FM5UIModel *model, const wchar_t *background);
/* Pure navigation, shared by keyboard, mouse and XInput. */
BOOL FM5NativeUIAction(FM5UIModel *model, unsigned action);
/* Offscreen render for visual inspection; no host/game/window creation. */
HRESULT FM5RenderNativeUIPreview(FM5UIModel *model, const wchar_t *background,
                               const wchar_t *output_bmp, unsigned width, unsigned height);
#ifdef __cplusplus
}
#endif
#endif
