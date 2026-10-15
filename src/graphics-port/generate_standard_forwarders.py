from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json,re
r=project_root(__file__);rows=json.loads((r/'analysis/graphics-port/context1-sdk-methods.json').read_text());text=[];assign=[]
special={'QueryInterface','AddRef','Release','GetDevice','IASetPrimitiveTopology','IASetIndexBuffer','DrawIndexed','DrawIndexedInstanced','DrawInstanced'}
for row in rows:
 old=row['old_slot'];az=row['az_slot'];name=row['name'];ret=row['return'];ps=row['params'][1:]
 if name in special:continue
 names=[];types=[]
 for p in ps:
  noarray=re.sub(r'\[[^\]]*\]','',p);m=re.search(r'(\w+)\s*$',noarray);n=m.group(1);names.append(n);types.append(p[:m.start()]+p[m.end():])
 params=', '.join(['PerformanceContext *context']+ps);targettypes=', '.join(['void *']+types);args=', '.join(['context->native']+names)
 text.append(f'static {ret} forward_{old}({params}) {{\n    typedef {ret} (*Target)({targettypes});\n    '+('return 'if ret!='void' else '')+f'((Target)(*(void ***)context->native)[{az}])({args});\n}}\n')
 assign.append(f'context->inline_methods[{old}] = (void *)forward_{old}; /* {name} -> {az} */')
(r/'src/graphics-port/fm5_context_standard_forwarders.inc').write_text('\n'.join(text));(r/'src/graphics-port/fm5_context_standard_assign.inc').write_text('\n'.join(assign)+'\n');print(len(assign),'typed forwarding methods generated')
