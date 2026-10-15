#include "fm5_native_loading.h"
#include <stdio.h>
static unsigned checks,native_calls,paint_calls;static HDC expected_dc;static RECT expected_rect={1,2,3,4};
static void native_draw(void *self,HDC *dc,const RECT *rect){
    if(self&&dc==&expected_dc&&rect==&expected_rect)native_calls++;
}
static void native_paint(HWND window,void *callback){
    void *closure=*(void**)((BYTE*)callback+0x38);paint_calls+=(window==(HWND)(uintptr_t)123);
    ((void(*)(void*,HDC*,const RECT*))(*(void***)closure)[2])(closure,&expected_dc,&expected_rect);
}

#define CHECK(v) do{checks++;if(!(v)){fprintf(stderr,"FAIL %u: %s\n",checks,#v);return 1;}}while(0)
int wmain(int argc,wchar_t **argv){unsigned p=99;uint64_t i,j;int left,right;
    CHECK(!FM5PreparationPercent(0,0,&p));CHECK(p==0);CHECK(!FM5PreparationPercent(1,1,NULL));
    CHECK(FM5PreparationPercent(1,3,&p));CHECK(p==33);CHECK(FM5PreparationPercent(2,3,&p));CHECK(p==66);
    CHECK(FM5PreparationPercent(UINT64_MAX-1,UINT64_MAX,&p));CHECK(p==99);
    CHECK(FM5PreparationPercent(UINT64_MAX,UINT64_MAX,&p));CHECK(p==100);
    CHECK(FM5PreparationPercent(UINT64_MAX,1,&p));CHECK(p==100);
    for(j=1;j<100;j++)for(i=0;i<=j;i++){CHECK(FM5PreparationPercent(i,j,&p));CHECK(p==(unsigned)(i*100/j));}
    CHECK(!FM5PreparationSweep(0,0,&left,&right));CHECK(!FM5PreparationSweep(100,0,NULL,&right));
    for(i=0;i<=3600;i+=7){CHECK(FM5PreparationSweep(621,i,&left,&right));CHECK(left>=0&&right<=621&&right>left);}
    CHECK(FM5PreparationSweep(621,0,&left,&right));CHECK(left==0&&right==124);
    CHECK(FM5PreparationSweep(621,900,&left,&right));CHECK(right==621);
    CHECK(FM5PreparationSweep(1,UINT64_MAX,&left,&right));CHECK(left==0&&right==1);
    {void *vtable[3]={NULL,NULL,(void*)native_draw};void *closure[1]={vtable};BYTE callback[0x40]={0};
     *(void**)(callback+0x38)=closure;
     FM5TestComposeNativePainter((HWND)(uintptr_t)123,callback,native_paint);
     CHECK(native_calls==1&&paint_calls==1);CHECK(*(void**)(callback+0x38)==closure);CHECK(closure[0]==vtable);}
    if(argc==3)CHECK(SUCCEEDED(FM5RenderLoadingPreview(argv[1],argv[2],37,100)));
    printf("{\"checks\":%u,\"ok\":true}\n",checks);return 0;
}
