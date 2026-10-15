#include "fm5_resource.h"
#include <stdio.h>
#include <string.h>
static unsigned called;
static HRESULT WINAPI backend(void *device,const void *description,const void *initial,void **out) {
    (void)device;(void)description;(void)initial;(void)out;++called;return S_OK;
}
int main(void) {
    union {void *pointer;HRESULT (WINAPI *call)(void *,const void *,const void *,void **);} method;
    method.call=backend;void *table[66]={0};table[3]=method.pointer;table[62]=method.pointer;table[64]=method.pointer;table[65]=method.pointer;void *device=table;
    DWORD desc[7]={16,0,0,0,0,0,0},original[7];memcpy(original,desc,sizeof(desc));
    void *out=(void *)1;HRESULT hr=FM5CreateLegacyResource(&device,3,desc,NULL,&out);
    BOOL passed=hr==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && out==NULL && called==0 && !memcmp(desc,original,sizeof(desc));
    passed=passed && FM5CreateLegacyResource(&device,3,desc,NULL,NULL)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && called==0;
    out=(void *)1;
    passed=passed && FM5CreateLegacyPlacementBuffer(&device,desc,NULL,&out)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && out==NULL && called==0;
    passed=passed && FM5CreateLegacyPlacementBuffer(&device,desc,NULL,NULL)==E_INVALIDARG && called==0;
    DWORD texture[12]={1920,1080,1,1,26,2,0,0,0x28,0,0,0};out=(void *)1;
    passed=passed && FM5CreateLegacyPlacementTexture2D(&device,texture,1,2,NULL,&out)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && out==NULL && called==0;
    passed=passed && FM5CreateLegacyPlacementTexture2D(&device,texture,1,2,NULL,NULL)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && called==0;
    passed=passed && FM5CreateLegacyPlacementTexture3D(&device,texture,13,0,NULL,&out)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && !out && called==0;
    passed=passed && FM5CreateLegacyPlacementTexture3D(&device,texture,13,0,NULL,NULL)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && called==0;
    void *wrong_resource=table;table[12]=method.pointer;
    passed=passed && FM5AdoptLegacyTexture2D(&wrong_resource)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && called==0;
    printf("{\"passed\":%s,\"hresult\":\"0x%08lx\",\"backend_calls\":%u,\"test_mode\":false}\n",passed?"true":"false",(unsigned long)hr,called);
    return passed?0:1;
}
