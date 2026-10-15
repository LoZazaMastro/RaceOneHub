from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import re,json,sys
r=project_root(__file__);add_python_libraries(__file__)
import pefile
pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'));data=pe.__data__
out=[]
for m in re.finditer(rb'[\x20-\x7e]{8,}',data):
 txt=m.group().decode()
 if any(s.lower() in txt.lower() for s in ['fence','waitforgpu','GPUwait','waitgpu','GPUSync','blockuntilgpu','SetPredication','insertfence','gpu signal','gpu value']):
  out.append({'rva':hex(pe.get_rva_from_offset(m.start())),'text':txt})
(r/'analysis/graphics-port/fm5-sync-strings.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
