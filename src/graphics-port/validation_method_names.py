from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import re,json
r=project_root(__file__);out=[]
for name in ['umd_d.dll','umd_v.dll','umd_i.dll']:
 path=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/name;raw=path.read_bytes();ss=[]
 for regex in [rb'[\x20-\x7e]{6,}',rb'(?:[\x20-\x7e]\x00){6,}']:
  for m in re.finditer(regex,raw):
   s=m.group().decode('utf-16le' if b'\x00' in m.group() else 'ascii')
   if re.fullmatch(r'(?:VS|PS|CS|DS|HS|GS|IA|OM|RS|SO|Draw|Dispatch|Map|Unmap|Get|Set|Clear|Copy|Update|Begin|End|Finish|Execute|Insert)[A-Za-z0-9_]{1,60}',s):ss.append({'offset':hex(m.start()),'name':s})
 out.append({'module':name,'strings':ss});print(name,len(ss),ss[:20])
(r/'analysis/graphics-port/umd-validation-method-names.json').write_text(json.dumps(out,indent=2))
