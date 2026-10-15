#include <assert.h>
#include <stdio.h>
#include "fm5_performance_context.h"
static void *expected,*expected_object;static UINT expected_slot;static unsigned expected_target,calls;static ULONG refs=1;
static ULONG nr(void*self){assert(self==expected&&refs);return --refs;}
static void record(unsigned target,void*self,UINT slot,UINT count,void *const *objects){assert(target==expected_target&&self==expected&&slot==expected_slot&&count==1&&objects&&objects[0]==expected_object);++calls;}
#define REC(N) static void t##N(void*self,UINT slot,UINT count,void *const *objects){record(N,self,slot,count,objects);}
REC(0) REC(1) REC(2) REC(3) REC(4) REC(5) REC(6) REC(7) REC(8) REC(9) REC(10) REC(11) REC(12) REC(13) REC(14) REC(15) REC(16) REC(17)
static void wait(void*self,void*resource){assert(self==expected&&resource==expected_object);++calls;}
static void vertex(void*self,UINT slot,UINT count,void *const *buffers,const UINT*strides,const UINT*offsets){assert(self==expected&&slot==15&&count==1&&buffers&&buffers[0]==expected_object&&strides&&*strides==16&&offsets&&*offsets==0);++calls;}
int main(void){void *table[270]={0};void **object=table;void*proxy;unsigned i,j;const unsigned indices[18]={0x11,9,11,8,0x19,0x1a,0x17,0x1f,0x20,0x47,0x43,0x46,0x3e,0x3b,0x3d,0x42,0x3f,0x41};void *targets[18]={(void*)t0,(void*)t1,(void*)t2,(void*)t3,(void*)t4,(void*)t5,(void*)t6,(void*)t7,(void*)t8,(void*)t9,(void*)t10,(void*)t11,(void*)t12,(void*)t13,(void*)t14,(void*)t15,(void*)t16,(void*)t17};
expected=&object;table[2]=(void*)nr;assert(FM5WrapPerformanceContext(expected,&proxy)==S_OK);
for(i=0;i<18;++i)table[indices[i]]=targets[i];
for(i=0;i<18;++i)for(j=0;j<2;++j){expected_target=i;expected_slot=j?127:0;expected_object=j?(void*)(ULONG_PTR)(0x12340000+i*16):NULL;((void(*)(void*,UINT,void*))(*(void***)proxy)[0x9b+i])(proxy,expected_slot,expected_object);}
expected_object=(void*)0x12340000;table[0x97]=(void*)wait;((void(*)(void*,void*))(*(void***)proxy)[0x92])(proxy,expected_object);
table[0x13]=(void*)vertex;((void(*)(void*,UINT,void*,UINT))(*(void***)proxy)[0xad])(proxy,15,expected_object,16);
assert(calls==38);assert(((ULONG(*)(void*))(*(void***)proxy)[2])(proxy)==0&&refs==0);puts("PASS: 18 single CB/SRV/sampler translations, count=1, stage/slot identity, null object, resource wait target, single vertex buffer with stride/zero offset");return 0;}
