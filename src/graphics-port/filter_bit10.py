from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);js=json.loads((r/'analysis/graphics-port/umd-context-state678-refs.json').read_text());out=[]
for x in js:
 line=next(y for y in x['asm'] if y.startswith(x['site'].replace('0x','')+':'))
 if any(p in line for p in [', 0x10',', 0xef']):out.append(x);print('\n'.join(x['asm']))
(r/'analysis/graphics-port/umd-bit10-consumers.json').write_text(json.dumps(out,indent=2))
