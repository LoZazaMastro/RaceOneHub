#include "../../fm5_gamepad_diagnostics.h"
#include <stdio.h>
int main(void){HRESULT hr=FM5SetGamepadVibration(NULL,NULL);BOOL installed=FM5InstallGamepadVibrationHook();BOOL passed=hr==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT) && !installed;printf("{\"passed\":%s,\"production_guard\":true,\"hr\":\"0x%08lx\",\"installed_outside_host\":%s}\n",passed?"true":"false",(unsigned long)hr,installed?"true":"false");return passed?0:1;}
