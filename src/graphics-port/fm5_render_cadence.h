#ifndef FM5_RENDER_CADENCE_H
#define FM5_RENDER_CADENCE_H
#include <windows.h>
#include <stddef.h>
/* Prepare never writes the guest. Commit requires the same exclusively owned,
   writable mapping and no intervening changes to the validated instruction sites.
   This allows the caller to validate language/profile/cadence before any write. */
typedef struct FM5RenderCadencePlan {
    unsigned char *image;
    UINT target;
    BOOL ready;
    void *wait_constant; /* Prepared immutable storage; committed lifetime is process. */
    INT32 wait_displacement;
} FM5RenderCadencePlan;
HRESULT FM5PrepareRenderCadence(unsigned char *image, size_t size, UINT target,
    FM5RenderCadencePlan *plan);
void FM5CommitRenderCadence(FM5RenderCadencePlan *plan);
void FM5DiscardRenderCadence(FM5RenderCadencePlan *plan);
#ifdef FM5_RENDER_CADENCE_TESTING
void FM5CadenceTestFailAllocation(BOOL enabled);
#endif
/* No enum or queue backpressure changes. Targets30/60 retain the original
   renderer service wait;120 changes only its private period, never shared data. */
HRESULT FM5ValidateRenderCadence(const unsigned char *image, size_t size, UINT target);
HRESULT FM5ApplyRenderCadence(unsigned char *image, size_t size, UINT target);
#endif
