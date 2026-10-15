from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json,re
r=project_root(__file__);rows=json.loads((r/'analysis/graphics-port/context1-sdk-methods.json').read_text());assigned=(r/'src/graphics-port/fm5_context_standard_assign.inc').read_text();rows=[x for x in rows if x['old_slot'] not in [76,97] and f'forward_{x["old_slot"]};'in assigned]
pre=['#define CINTERFACE','#include <d3d11_1.h>','#include <assert.h>','#include <stdio.h>','#include "fm5_performance_context.h"','static void *expected; static ULONG refs=1; static unsigned calls;','static HRESULT nq(void*self,const GUID*g,void**out){(void)self;(void)g;*out=NULL;return E_NOINTERFACE;}','static ULONG nr(void*self){assert(self==expected&&refs);return --refs;}','static HRESULT WINAPI desc(void*r,void*d){(void)r;ZeroMemory(d,48);((UINT*)d)[5]=2;return S_OK;}'];defs=[];main=['int main(void){','void *table[270]={0}; void **object=table; void *proxy; expected=&object;table[0]=(void*)nq;table[2]=(void*)nr;','FM5ConfigureContextResolve(desc);','assert(FM5WrapPerformanceContext(expected,&proxy)==S_OK);']
for row in rows:
 old=row['old_slot'];az=row['az_slot'];ret=row['return'];ps=row['params'][1:];names=[];types=[];vals=[]
 for k,p in enumerate(ps):
  noarray=re.sub(r'\[[^\]]*\]','',p);m=re.search(r'(\w+)\s*$',noarray);names.append(m.group(1));typ=p[:m.start()]+p[m.end():];typ=re.sub(r'\[[^\]]*\]','*',typ).strip();types.append(typ)
  val=f'({typ})(ULONG_PTR)0x{0x100000+old*256+k*16:x}'if '*'in typ else (f'({typ}){k+1}.375f' if typ=='FLOAT' else f'({typ}){(old*5+k)%127+1}')
  vals.append(val)
 decl=', '.join(['void *self']+ps);checks=' '.join(f'assert({n}=={v});'for n,v in zip(names,vals));returnval='27.125f'if ret=='FLOAT'else f'({ret})37';defs.append(f'static {ret} t_{old}({decl}){{assert(self==expected);{checks}++calls;'+(''if ret=='void'else f'return {returnval};')+'}')
 main.append(f'table[{az}]=(void*)t_{old};')
 ftype=', '.join(['void*']+types);args=', '.join(['proxy']+vals);call=f'(({ret}(*)({ftype}))(*(void***)proxy)[{old}])({args})';main.append(call+';'if ret=='void'else f'assert({call}=={returnval});')
main+= [f'assert(calls=={len(rows)});','assert(((ULONG(*)(void*))(*(void***)proxy)[2])(proxy)==0&&refs==0);',f'puts("PASS: {len(rows)} typed standard methods, every scalar/pointer/stack argument and return value");','return 0;}']
(r/'src/graphics-port/test_context_generated.c').write_text('\n'.join(pre+defs+main)+'\n');print(len(rows))
