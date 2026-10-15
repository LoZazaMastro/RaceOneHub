#include "fm5_shape.h"
#include <stdio.h>
int main(void){DWORD code=0;HRESULT hr=0;__try{FM5ExecuteShapeGraph(NULL,NULL);}__except(EXCEPTION_EXECUTE_HANDLER){code=GetExceptionCode();}/* Guard precedes pointer validation in an unrelated host. */hr=FM5TranslateShapeGraph(NULL,NULL,0);BOOL ok=code==0xe05f0058 && hr==S_OK;printf("{\"passed\":%s,\"test_mode\":false}\n",ok?"true":"false");return ok?0:1;}
