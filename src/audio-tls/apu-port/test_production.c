#include "fm5_apu.h"
#include <stdio.h>
int main(void){void *p=(void *)1;UINT32 a=1;BOOL ok=FM5EnsureApuDefaultHeap()==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);ok=ok&&FM5ApuAlloc(&p,&a,4,4,1)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT)&&!p&&!a;p=(void *)1;ok=ok&&FM5AcpHalCreate(&p)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT)&&!p;BYTE data[96];ok=ok&&FM5AcpHalAllocateShapeContexts(data)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);printf("{\"passed\":%s,\"test_mode\":false}\n",ok?"true":"false");return ok?0:1;}
