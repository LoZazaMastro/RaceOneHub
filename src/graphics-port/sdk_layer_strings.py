from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re,json,uuid
r=project_root(__file__);add_python_libraries(__file__);import pefile
p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/d3d11_1SDKLayers.dll';pe=pefile.PE(str(p));raw=pe.__data__;out=[]
for pat,enc in [(rb'[\x20-\x7e]{7,}','ascii'),(rb'(?:[\x20-\x7e]\x00){7,}','utf-16-le')]:
 for m in re.finditer(pat,raw):
  s=m.group().decode(enc)
  if any(x.lower() in s.lower() for x in ['PerformanceContext','DisableCommand','CommandLists','deferred','ContextFlags','recording']):out.append({'rva':hex(pe.get_rva_from_offset(m.start())),'text':s})
print(json.dumps(out,indent=2));(r/'analysis/graphics-port/sdk-layer-performance-strings.json').write_text(json.dumps(out,indent=2))
