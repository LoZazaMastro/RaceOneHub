from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);js=json.loads((r/'analysis/graphics-port/fm5-context-call-candidates.json').read_text());out=[]
for g in js:
 if g['slot'] not in ['0x1b','0x1c','0x1d']:continue
 for x in g['sites']:
  n=int(x['rva'],16)
  if 0x69f000<=n<=0x720000:
   out.append(dict(x,slot=g['slot']))
   print('slot',g['slot'],'rva',x['rva']);print('\n'.join(x['assembly'][-22:]))
(r/'analysis/graphics-port/fm5-render-query-candidates.json').write_text(json.dumps(out,indent=2))
