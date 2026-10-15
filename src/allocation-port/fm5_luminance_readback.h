#ifndef FM5_LUMINANCE_READBACK_H
#define FM5_LUMINANCE_READBACK_H
#include <windows.h>
#ifdef __cplusplus
extern "C" {
#endif
/* Optional diagnostic, enabled only by FM5_LUMINANCE_READBACK=1.
 * Call after genuine native12a6d0; pass its unchanged returned abstract list. */
__declspec(dllexport) void WINAPI FM5ReadbackAfterPreparation(void *context,void *abstract_list);
/* Forwarders for ONLY nativeEE44A/EEABC load-R10 callsites. Seven args total. */
__declspec(dllexport) void WINAPI FM5ReadbackCopyTextureRegion(void *list,const void *destination,UINT x,UINT y,UINT z,const void *source,const void *box);
/* Queue observer calls after exactly one genuine ExecuteCommandLists. */
void WINAPI FM5ReadbackSubmitted(void *queue,UINT count,void *const *lists);
#ifdef __cplusplus
}
#endif
#endif
