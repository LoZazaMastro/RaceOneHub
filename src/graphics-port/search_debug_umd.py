from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import re,json,sys
r=project_root(__file__);add_python_libraries(__file__);import pefile
out=[]
for n in ['umd_d.dll','umd_v.dll','umd_i.dll']:
 p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/n),fast_load=True)
 for pat,enc in [(rb'[\x20-\x7e]{6,}','ascii'),(rb'(?:[\x20-\x7e]\x00){6,}','utf-16-le')]:
  for m in re.finditer(pat,p.__data__):
   s=m.group().decode(enc)
   if any(t.lower() in s.lower() for t in ['Automatic','PartialFlush','PerformanceContext','PrivateDataInterface','SetAuto','AutoFlush']):
    row={'file':n,'rva':hex(p.get_rva_from_offset(m.start())),'text':s};out.append(row);print(json.dumps(row))
(r/'analysis/graphics-port/umd-debug-abi-strings.json').write_text(json.dumps(out,indent=2))
