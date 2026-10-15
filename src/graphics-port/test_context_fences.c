#include <assert.h>
#include <stdio.h>
#include "fm5_performance_context.h"
static void *expected;static ULONG refs=1;static unsigned calls;static UINT kind;static ULONG release(void*self){assert(self==expected&&refs);return --refs;}
static void caches(void*self,UINT flags,UINT64 address,UINT64 bytes){assert(self==expected&&flags==0x1234&&address==0xfedcba9876543210ull&&bytes==0x123456789abcdef0ull);++calls;}
static void idle(void*self,UINT flags){assert(self==expected&&flags==0);++calls;}
static UINT64 fence(void*self,UINT flags){assert(self==expected&&flags==0);++calls;return 0xfedcba9876543210ull;}
static void wait(void*self,UINT flags,UINT64 token){assert(self==expected&&flags==1&&token==0xfedcba9876543210ull);++calls;}
static UINT type(void*self){assert(self==expected);return kind;}
static void remapcb(void*self,UINT a,UINT b,UINT c,UINT d){assert(self==expected&&a==1&&b==13&&c==4&&d==3);++calls;}
static void remapsrv(void*self,UINT a,UINT b,UINT c,UINT d){assert(self==expected&&a==5&&b==127&&c==0&&d==123);++calls;}
int main(void){void*table[270]={0};void**native=table;void*proxy,*adopted;expected=&native;table[2]=(void*)release;table[0x98]=(void*)caches;table[0x99]=(void*)idle;table[0x9a]=(void*)fence;table[0x9b]=(void*)wait;table[0x70]=(void*)type;table[0x9c]=(void*)remapcb;table[0x9d]=(void*)remapsrv;assert(FM5WrapPerformanceContext(expected,&proxy)==S_OK);assert(FM5WrapPerformanceContext(proxy,&adopted)==S_OK&&adopted==proxy);
((void(*)(void*,UINT,UINT64,UINT64))(*(void***)proxy)[0x93])(proxy,0x1234,0xfedcba9876543210ull,0x123456789abcdef0ull);
((void(*)(void*,UINT))(*(void***)proxy)[0x94])(proxy,0);
assert(((UINT64(*)(void*,UINT))(*(void***)proxy)[0x95])(proxy,0)==0xfedcba9876543210ull);
((void(*)(void*,UINT,UINT64))(*(void***)proxy)[0x96])(proxy,1,0xfedcba9876543210ull);
kind=0;((void(*)(void*,UINT,UINT,UINT,UINT))(*(void***)proxy)[0x97])(proxy,1,13,4,3);((void(*)(void*,UINT,UINT,UINT,UINT))(*(void***)proxy)[0x98])(proxy,5,127,0,123);assert(calls==4);
kind=1;((void(*)(void*,UINT,UINT,UINT,UINT))(*(void***)proxy)[0x97])(proxy,1,13,4,3);((void(*)(void*,UINT,UINT,UINT,UINT))(*(void***)proxy)[0x98])(proxy,5,127,0,123);assert(calls==6);
assert(((ULONG(*)(void*))(*(void***)proxy)[2])(proxy)==0&&refs==0);puts("PASS: GPU cache/idle/fence mappings, full-width U64 tokens, deferred inheritance and authentic immediate no-op split, owned proxy adoption");return 0;}
