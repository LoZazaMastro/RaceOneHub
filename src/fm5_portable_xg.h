#ifndef FM5_PORTABLE_XG_H
#define FM5_PORTABLE_XG_H
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Host eight-method ABI backed exclusively by the supplied FM5 factory.
 * No library search, other game's DLL, or embedded original implementation. */
__declspec(dllexport) HRESULT WINAPI FM5PortableCreateHostTextureComputer(
    const void *description,void **out,void *original_factory);
#ifdef __cplusplus
}
#endif
#endif
