#include "fm5_render_cadence.h"
#include <string.h>
#include <stdint.h>
#include <limits.h>

static const unsigned char renderer_wait_guard[] = {
    0x40,0x53,0x48,0x83,0xec,0x30,0xc5,0xf8,0x29,0x74,0x24,0x20,0xc5,0xfb,0x10,0x35,0x3c,0xa3,0x5c,0x01,0x48,0x8d,0x99,0x40,0x04,0x00,0x00,0x0f,0x1f,0x44,0x00,0x00,0x48,0x8b,0xcb,0xff,0x53,0x58,0xc5,0xfb,0x5c,0x0b,0xc5,0xf9,0x2f,0xce,0x72,0xf0,0x48,0x8b,0xcb,0xff,0x53,0x58,0xc5,0xf8,0x28,0x74,0x24,0x20,0xc5,0xfb,0x11,0x03,0x48,0x83,0xc4,0x30,0x5b,0xc3};

#ifdef FM5_RENDER_CADENCE_TESTING
static BOOL fail_allocation;
void FM5CadenceTestFailAllocation(BOOL enabled) { fail_allocation = enabled; }
#endif

static void *allocate_near(uintptr_t origin, INT32 *displacement)
{
    SYSTEM_INFO info;
    MEMORY_BASIC_INFORMATION region;
    uintptr_t low, high, cursor;
    GetSystemInfo(&info);
    low = origin > INT32_MAX ? origin - INT32_MAX : (uintptr_t)info.lpMinimumApplicationAddress;
    high = origin < UINTPTR_MAX - INT32_MAX ? origin + INT32_MAX : UINTPTR_MAX;
    if (high > (uintptr_t)info.lpMaximumApplicationAddress) high = (uintptr_t)info.lpMaximumApplicationAddress;
#ifdef FM5_RENDER_CADENCE_TESTING
    if (fail_allocation) return NULL;
#endif
    cursor = low;
    while (cursor < high && VirtualQuery((const void *)cursor, &region, sizeof(region))) {
        uintptr_t end = (uintptr_t)region.BaseAddress + region.RegionSize;
        if (end <= cursor) break;
        if (region.State == MEM_FREE) {
            uintptr_t aligned = (cursor + info.dwAllocationGranularity - 1) & ~((uintptr_t)info.dwAllocationGranularity - 1);
            if (aligned >= cursor && aligned < high && aligned < end && end - aligned >= sizeof(double)) {
                void *memory = VirtualAlloc((void *)aligned, sizeof(double), MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
                if (memory) {
                    intptr_t delta = (intptr_t)((uintptr_t)memory - origin);
                    DWORD previous;
                    *(double *)memory = 1.0 / 120.0;
                    if (delta >= INT32_MIN && delta <= INT32_MAX &&
                        VirtualProtect(memory, sizeof(double), PAGE_READONLY, &previous)) {
                        *displacement = (INT32)delta;
                        return memory;
                    }
                    VirtualFree(memory, 0, MEM_RELEASE);
                }
            }
        }
        cursor = end;
    }
    return NULL;
}


HRESULT FM5ValidateRenderCadence(const unsigned char *image, size_t size, UINT target)
{
    static const unsigned char getter_guard[] = {
        0x48,0x8b,0x89,0xb8,0,0,0,0x48,0x8b,0x01,0xff,0x90,0x88,0x01,0,0,
        0x48,0x8d,0xae,0x90,0x01,0,0,0xb9,0x01,0,0,0,0x3b,0xc1,0x75,0x38};
    static const unsigned char parity_guard[] = {
        0x44,0x0f,0xb6,0xb6,0x20,0x01,0,0,0x41,0x80,0xe6,0x01};
    static const unsigned char dispatch_guard[] = {
        0x45,0x84,0xf6,0x0f,0x85,0xa6,0,0,0,0x45,0x84,0xff,0x0f,0x85,0x9d,0,0,0};
    if (!image) return E_POINTER;
    if (target != 30 && target != 60 && target != 120) return E_INVALIDARG;
    if (size != 0x279d000 ||
        memcmp(image + 0x5a898f, getter_guard, sizeof(getter_guard)) ||
        memcmp(image + 0x5a89d9, parity_guard, sizeof(parity_guard)) ||
        memcmp(image + 0x5a8a6c, dispatch_guard, sizeof(dispatch_guard)) ||
        memcmp(image + 0x7141b0, renderer_wait_guard, sizeof(renderer_wait_guard)) ||
        *(const double *)(image + 0x1cde500) != 1.0 / 60.0)
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    return S_OK;
}

HRESULT FM5PrepareRenderCadence(unsigned char *image, size_t size, UINT target,
    FM5RenderCadencePlan *plan)
{
    HRESULT status;
    if (!plan) return E_POINTER;
    memset(plan, 0, sizeof(*plan));
    status = FM5ValidateRenderCadence(image, size, target);
    if (FAILED(status)) return status;
    if (target == 120) {
        plan->wait_constant = allocate_near((uintptr_t)(image + 0x7141c4), &plan->wait_displacement);
        if (!plan->wait_constant) {
            memset(plan, 0, sizeof(*plan));
            return E_OUTOFMEMORY;
        }
    }
    plan->image = image;
    plan->target = target;
    plan->ready = TRUE;
    return S_OK;
}

void FM5CommitRenderCadence(FM5RenderCadencePlan *plan)
{
    static const unsigned char full_rate[] = {
        0x45,0x33,0xf6,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90,0x90};
    if (!plan || !plan->ready) return;
    if (plan->target >= 60)
        memcpy(plan->image + 0x5a89d9, full_rate, sizeof(full_rate));
    if (plan->target == 120)
        memcpy(plan->image + 0x7141c0, &plan->wait_displacement, sizeof(plan->wait_displacement));
    /* The RIP operand now owns this immutable allocation for the guest lifetime. */
    memset(plan, 0, sizeof(*plan));
}

void FM5DiscardRenderCadence(FM5RenderCadencePlan *plan)
{
    if (!plan) return;
    if (plan->wait_constant) VirtualFree(plan->wait_constant, 0, MEM_RELEASE);
    memset(plan, 0, sizeof(*plan));
}

HRESULT FM5ApplyRenderCadence(unsigned char *image, size_t size, UINT target)
{
    FM5RenderCadencePlan plan;
    HRESULT status = FM5PrepareRenderCadence(image, size, target, &plan);
    if (FAILED(status)) return status;
    FM5CommitRenderCadence(&plan);
    return S_OK;
}
