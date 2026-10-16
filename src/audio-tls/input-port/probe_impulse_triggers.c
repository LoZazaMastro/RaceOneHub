#include "../../fm5_impulse_triggers.h"
#include <stdio.h>
#include <string.h>
/* Lists the pads Windows.Gaming.Input sees, as the Bridge's impulse trigger
   worker does. With --pulse, both triggers of the first pad rumble for half a
   second. Not part of any game process. */
int main(int argc,char **argv) {
    const FM5TriggerDevice *device=FM5ImpulseTriggerDevice();
    void *ids[8];FM5TriggerInput inputs[8];UINT count=0;
    HRESULT hr=device->open();
    if(FAILED(hr)){printf("{\"passed\":false,\"open_hr\":\"0x%08lx\"}\n",(unsigned long)hr);return 1;}
    for(UINT wait=0;wait<20 && !count;++wait){count=device->pads(ids,inputs,8);if(!count)Sleep(100);}
    printf("{\"passed\":true,\"pads\":%u,\"inputs\":[",count);
    for(UINT i=0;i<count;++i)printf("%s{\"buttons\":\"0x%04x\",\"left\":%u,\"right\":%u}",i?",":"",inputs[i].buttons,inputs[i].left,inputs[i].right);
    if(argc==2 && !strcmp(argv[1],"--pulse") && count) {
        const float on[4]={0,0,0.6f,0.6f},off[4]={0};
        HRESULT pulse=device->vibrate(ids[0],on);Sleep(500);HRESULT stop=device->vibrate(ids[0],off);
        printf("],\"pulse_hr\":\"0x%08lx\",\"stop_hr\":\"0x%08lx\"}\n",(unsigned long)pulse,(unsigned long)stop);
        return SUCCEEDED(pulse) && SUCCEEDED(stop)?0:1;
    }
    printf("]}\n");return 0;
}
