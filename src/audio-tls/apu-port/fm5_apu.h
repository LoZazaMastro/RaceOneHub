#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define FM5_APU_HEAP1_BYTES 0x04000000u
#define FM5_APU_HEAP2_BYTES 0x00114000u
#define FM5_APU_HEAP1_ADDRESS 0x01000000u
#define FM5_APU_HEAP2_ADDRESS 0x05000000u
HRESULT WINAPI FM5EnsureApuDefaultHeap(void);
HRESULT WINAPI FM5AcpHalCreate(void **out);
HRESULT WINAPI FM5ApuAlloc(void **cpu,UINT32 *apu,UINT32 bytes,UINT32 alignment,UINT32 heap_type);
HRESULT WINAPI FM5AcpHalAllocateShapeContexts(void *inout_contexts);
#ifdef FM5_APU_TEST
typedef struct FM5APUHeap {void *cpu;UINT32 bytes;UINT32 apu;void *tree;SIZE_T entries;} FM5APUHeap;
typedef struct FM5APUTestBackends {
 HRESULT (WINAPI *create)(UINT32,UINT32);
 HRESULT (WINAPI *alloc)(void **,UINT32 *,UINT32,UINT32,UINT32);
 HRESULT (WINAPI *shape)(void *);
 HRESULT (WINAPI *hal_create)(void **);
 BYTE *ready;FM5APUHeap *heaps;
} FM5APUTestBackends;
void FM5APUSetTestBackends(const FM5APUTestBackends *backend);
#endif
