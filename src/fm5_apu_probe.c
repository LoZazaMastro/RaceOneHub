#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "audio-tls/apu-port/fm5_apu.h"

int FM5RunApuNativeProbe(void) {
    static const struct {DWORD rva;BYTE bytes[16];} guards[] = {
        {0xaed20,{0x40,0x53,0x48,0x83,0xec,0x20,0x48,0x8b,0xd9,0x48,0x8d,0x0d,0x80,0xa4,0x26,0x00}},
        {0xaed90,{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x20,0x8b}},
        {0xaf230,{0x48,0x85,0xc9,0x0f,0x84,0x3e,0x01,0x00,0x00,0x48,0x89,0x5c,0x24,0x08,0x48,0x89}}
    };
    BYTE *base = (BYTE *)GetModuleHandleW(NULL);
    UINT32 (WINAPI *to_apu)(void *);
    void *(WINAPI *to_cpu)(UINT32);
    void (WINAPI *release)(void *);
    HRESULT result;
    void *cpu[2] = {NULL,NULL};
    UINT32 apu[2] = {0,0};
    UINT pass, heap;
    for (UINT i = 0; i < 3; ++i)
        if (memcmp(base + guards[i].rva, guards[i].bytes, 16)) return 2;
    to_apu = (UINT32 (WINAPI *)(void *))(uintptr_t)(base + 0xaed20);
    to_cpu = (void *(WINAPI *)(UINT32))(uintptr_t)(base + 0xaed90);
    release = (void (WINAPI *)(void *))(uintptr_t)(base + 0xaf230);
    result = FM5EnsureApuDefaultHeap();
    if (FAILED(result)) {
        fprintf(stderr,"[fm5-apu-probe] initialization failed=%08lx\n",(unsigned long)result);
        return 3;
    }
    for (pass = 0; pass < 2; ++pass) {
        const UINT32 sizes[2] = {
            pass ? FM5_APU_HEAP1_BYTES : 0x600000,
            pass ? FM5_APU_HEAP2_BYTES : 0x80000
        };
        for (heap = 0; heap < 2; ++heap) {
            result = FM5ApuAlloc(&cpu[heap], &apu[heap], sizes[heap], 256, heap + 1);
            if (FAILED(result) || !cpu[heap]) goto failed;
            if (((uintptr_t)cpu[heap] & 255) || (apu[heap] & 255) ||
                to_apu(cpu[heap]) != apu[heap] || to_cpu(apu[heap]) != cpu[heap] ||
                to_apu((BYTE *)cpu[heap] + sizes[heap] - 1) != apu[heap] + sizes[heap] - 1 ||
                to_cpu(apu[heap] + sizes[heap] - 1) != (BYTE *)cpu[heap] + sizes[heap] - 1)
                goto failed;
            ((BYTE *)cpu[heap])[0] = 0x5a;
            ((BYTE *)cpu[heap])[sizes[heap] - 1] = 0xa5;
            if (*(BYTE *)to_cpu(apu[heap]) != 0x5a ||
                *(BYTE *)to_cpu(apu[heap] + sizes[heap] - 1) != 0xa5) goto failed;
            fprintf(stderr,"[fm5-apu-probe] pass=%u heap=%u bytes=%u cpu=%p apu=%08x mappings=PASS\n",
                    pass,heap+1,sizes[heap],cpu[heap],apu[heap]);
        }
        for (heap = 0; heap < 2; ++heap) {release(cpu[heap]);cpu[heap] = NULL;}
    }
    fprintf(stderr,"[fm5-apu-probe] PASS: both full capacities, bidirectional endpoint mappings, alignment and free/reallocate\n");
    return 0;
failed:
    fprintf(stderr,"[fm5-apu-probe] FAIL pass=%u heap=%u result=%08lx\n",pass,heap+1,(unsigned long)result);
    for (heap = 0; heap < 2; ++heap) if (cpu[heap]) release(cpu[heap]);
    return 4;
}
