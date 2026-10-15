#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "fm5_graphics_bridge.h"

typedef struct Mock {
    void **vtable;
    unsigned long refs;
    int kind;
} Mock;
static int scenario, calls;
static FM5CreateDescriptor observed;
static Mock dev, ctx;
static FM5HRESULT mock_query(void *self, const FM5Guid *iid, void **out) {
    Mock *obj = (Mock *)self;
    assert(iid->a == (obj->kind ? 0xc0bfa96c : 0xdb6f6ddb));
    if ((scenario == 2 && !obj->kind) || (scenario == 3 && obj->kind))
        return (FM5HRESULT)0x80004002L;
    ++obj->refs;
    *out = obj;
    return 0;
}
static unsigned long mock_release(void *self) {
    Mock *obj = (Mock *)self;
    assert(obj->refs);
    return --obj->refs;
}
static void *vtable[] = { (void *)mock_query, 0, (void *)mock_release };
static FM5HRESULT mock_factory(const FM5CreateDescriptor *desc, void **d, void **c) {
    ++calls;
    observed = *desc;
    if (scenario == 1) return (FM5HRESULT)0x8007000eL;
    dev.vtable = ctx.vtable = vtable;
    dev.kind = 0; ctx.kind = 1;
    dev.refs = ctx.refs = 1;
    *d = &dev;
    if (scenario == 4) { ctx.refs = 0; return 0; }
    *c = &ctx;
    return 0;
}
static FM5HRESULT invoke(unsigned flags, unsigned driver, unsigned sdk, unsigned count,
                         unsigned level, void **d, unsigned *selected, void **c) {
    return ForzaMastro5D3D11CreateDevice(0, driver, 0, flags, &level, count,
                                        sdk, d, selected, c);
}
int main(void) {
    void *d = (void *)1, *c = (void *)1;
    unsigned level = 0;
    int i;
    assert(sizeof(FM5UINT) == 4 && sizeof(FM5HRESULT) == 4);
    assert(offsetof(FM5CreateDescriptor, hardware_mask) == 24);
    assert(offsetof(FM5CreateDescriptor, reserved5) == 40);
    assert(FM5ConfigureGraphicsBridge(mock_factory, 0) == 0);
    assert(invoke(0x20, 1, 0x10002, 1, 0xb100, &d, &level, &c) < 0);
    assert(calls == 0 && !d && !c && !level);
    assert(FM5ConfigureGraphicsBridge(mock_factory, 1) == 0);
    assert(FM5ConfigureGraphicsBridge(mock_factory, 2) < 0);
    for (i=0; i<2; ++i) {
        scenario = 0;
        assert(invoke(i ? 0x8000020 : 0x20, 1, 0x10002, 1, 0xb100, &d, &level, &c) == 0);
        assert(d == &dev && c == &ctx && dev.refs == 1 && ctx.refs == 1);
        assert(level == 0xb100);
        assert(observed.version == 0x1000b && observed.flags == 0x4000000 && observed.hardware_mask == 0x3f);
        assert(!observed.reserved0 && !observed.reserved1 && !observed.reserved2 &&
               !observed.reserved3 && !observed.reserved4 && !observed.reserved5);
        mock_release(d); mock_release(c);
    }
    for (scenario=1; scenario<=4; ++scenario) {
        d = c = (void *)1; level = 123;
        assert(invoke(0x20, 1, 0x10002, 1, 0xb100, &d, &level, &c) < 0);
        assert(!d && !c && !level && !dev.refs && !ctx.refs);
    }
    i = calls;
    assert(invoke(0x20, 5, 0x10002, 1, 0xb100, &d, &level, &c) < 0);
    assert(invoke(0, 1, 0x10002, 1, 0xb100, &d, &level, &c) < 0);
    assert(invoke(0x20, 1, 0x1000b, 1, 0xb100, &d, &level, &c) < 0);
    assert(invoke(0x20, 1, 0x10002, 2, 0xb100, &d, &level, &c) < 0);
    assert(invoke(0x20, 1, 0x10002, 1, 0xb000, &d, &level, &c) < 0);
    assert(invoke(0x20, 1, 0x10002, 1, 0xb100, 0, &level, &c) < 0);
    assert(invoke(0x20, 1, 0x10002, 1, 0xb100, &d, &level, 0) < 0);
    assert(calls == i);
    assert(FM5ConfigureGraphicsBridge(0, 0) == 0);
    puts("PASS: argument shape, descriptor normalization, COM ownership, partial-failure rollback, refusal paths");
    return 0;
}
