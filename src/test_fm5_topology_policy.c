#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

HRESULT FM5TopologyPacket(UINT, uint64_t *);
HRESULT FM5ApplyNativeTopology(void *, UINT);
int main(void) {
    static const uint64_t packets[] = {
        0, UINT64_C(0x3ffc0009e00), UINT64_C(0x5ffc0009e00), UINT64_C(0x7ffc0009e00),
        UINT64_C(0x9ffc0009e00), UINT64_C(0xdffc0009e00)
    };
    __declspec(align(16)) unsigned char context[0x200];
    unsigned char expected[sizeof(context)];
    uint64_t packet;
    UINT topology;
    assert(FM5TopologyPacket(4, NULL) == E_POINTER);
    assert(FM5ApplyNativeTopology(NULL, 4) == E_POINTER);
    for (topology = 1; topology <= 5; ++topology) {
        uint32_t flags = 0xaabbcc00;
        memset(context, 0x7d, sizeof(context));
        memcpy(context + 0x168, &flags, sizeof(flags));
        memcpy(expected, context, sizeof(context));
        flags |= 0x46;
        memcpy(expected + 0x168, &flags, sizeof(flags));
        memcpy(expected + 0x170, &packets[topology], sizeof(packet));
        assert(FM5TopologyPacket(topology, &packet) == S_OK && packet == packets[topology]);
        assert(FM5ApplyNativeTopology(context, topology) == S_OK);
        assert(!memcmp(context, expected, sizeof(context)));
    }
    memcpy(expected, context, sizeof(context));
    for (topology = 0; topology <= 128; ++topology) {
        if (topology >= 1 && topology <= 5) continue;
        assert(FM5TopologyPacket(topology, &packet) == E_NOTIMPL && !packet);
        assert(FM5ApplyNativeTopology(context, topology) == E_NOTIMPL);
        assert(!memcmp(context, expected, sizeof(context)));
    }
    puts("PASS: 5 topology packets, exact fast-context writes, preserved unrelated bytes, 124 rejected enum values");
    return 0;
}
