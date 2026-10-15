#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <malloc.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
static volatile LONG heap_traces;

/* ERA attribute layout follows XWine1/SlimEra's public era.h. */
__declspec(dllexport) void *WINAPI XMemAlloc(SIZE_T bytes, uint64_t attributes) {
    unsigned int alignment_log2 = (unsigned int)((attributes >> 24) & 31);
    unsigned int memory_type = (unsigned int)((attributes >> 29) & 15);
    void *memory;
    if (memory_type || (attributes >> 33) || alignment_log2 > 30) {
        fprintf(stderr, "[fm5-xmem] unsupported attributes=%016llx bytes=%zu\n",
                (unsigned long long)attributes, bytes);
        SetLastError(ERROR_NOT_SUPPORTED);
        return NULL;
    }
    memory = _aligned_malloc(bytes, (SIZE_T)1 << (alignment_log2 < 4 ? 4 : alignment_log2));
    if (memory) memset(memory, 0, bytes);
    if (InterlockedIncrement(&heap_traces) <= 16)
        fprintf(stderr, "[fm5-xmem] heap attributes=%016llx bytes=%zu result=%p\n",
                (unsigned long long)attributes, bytes, memory);
    return memory;
}

__declspec(dllexport) void WINAPI XMemFree(void *memory, uint64_t attributes) {
    if (((attributes >> 29) & 15) || (attributes >> 33)) {
        RaiseException(0xe0f50004, EXCEPTION_NONCONTINUABLE, 0, NULL);
        return;
    }
    _aligned_free(memory);
}
