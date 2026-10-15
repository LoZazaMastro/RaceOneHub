#include "../../fm5_gamepad_diagnostics.h"
#include <stdio.h>
int main(void){HRESULT hr=FM5SetGamepadVibration(NULL,NULL);BOOL passed=hr==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);printf("{\"passed\":%s,\"production_guard\":true,\"hr\":\"0x%08lx\"}\n",passed?"true":"false",(unsigned long)hr);return passed?0:1;}
