from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);js=json.loads((r/'analysis/graphics-port/memory-counter-loop-candidates.json').read_text())
keep=[]
for item in js:
 a=item['assembly'];s='\n'.join(a)
 if len(a)>12 or any(': ret' in x or ': call' in x for x in a):continue
 if not any(': sub e' in x or ': sub r8d' in x or ': sub r9d' in x or ': sub r10d' in x or ': sub r11d' in x or ': sub r12d' in x for x in a):continue
 keep.append(item)
print(json.dumps(keep,indent=2));(r/'analysis/graphics-port/short-sub-counter-loops.json').write_text(json.dumps(keep,indent=2))
p=r/'analysis/rea/session/308-graphics-host-wait-xrefs.response.json'
print('hostwaitxrefs',json.loads(p.read_text())['result']['structuredContent']['raw_result'])
