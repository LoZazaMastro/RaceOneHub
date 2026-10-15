#include "fm5_fmask_bridge.h"
#include <assert.h>
#include <stdio.h>
HRESULT WINAPI FM5GetLegacyTexture2DDescription(void *resource,void *out48) {
    (void)resource;(void)out48;assert(0);return E_UNEXPECTED;
}
HRESULT WINAPI FM5CreateLegacyResource(void *self,UINT slot,const void *description,const void *data,void **out) {
    (void)self;(void)slot;(void)description;(void)data;(void)out;assert(0);return E_UNEXPECTED;
}
static HRESULT WINAPI foreign_method(void) { assert(0);return E_UNEXPECTED; }
int main(void) {
    void *table[8]={0},**device=table,*out=(void*)1;
    table[7]=(void*)foreign_method;
    assert(FM5CreateLegacyShaderResourceView(&device,NULL,NULL,&out)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT));
    assert(!out);
    puts("PASS: production FMASK guard rejects foreign native implementation before callback or metadata reads");
    return 0;
}
