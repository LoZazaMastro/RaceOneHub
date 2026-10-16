#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "fm5_gamepad_diagnostics.h"

/* The prepared host imports the Bridge, so this runs once the host image is
   mapped and relocated and before any of its own code. Only memory protection
   and pointer writes happen here; nothing loads or waits under the loader lock. */
BOOL WINAPI DllMain(HINSTANCE instance,DWORD reason,LPVOID reserved) {
    (void)instance;(void)reserved;
    if(reason==DLL_PROCESS_ATTACH)FM5InstallGamepadVibrationHook();
    return TRUE;
}
