#ifndef FM5_ALLOCATION_H
#define FM5_ALLOCATION_H
#include <windows.h>

/* Returns a genuine native object, owned by the caller. Before publication,
 * register legacy Buffer metadata: slot=3, extra=0, clear_misc_mask=0x40000000.
 * The caller retains ownership of placement memory through native GPU retirement.
 */
HRESULT WINAPI FM5CreateNativePlacementBuffer(void *native_device,
    const void *legacy_description28, void *placement, void **native_owned_out);
#endif
