from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re,uuid
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
for n in ['d3d11_x.dll','d3d11.dll','d3d11i.dll']:
 p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/n;pe=pefile.PE(str(p));print(n)
 for m in re.finditer(rb'[\x20-\x7e]{5,}',pe.__data__):
  if n=='d3d11_x.dll' or b'.dll' in m.group().lower() and any(x in m.group().lower() for x in [b'd3d',b'ddi',b'graphics',b'xbox']): print(hex(pe.get_rva_from_offset(m.start())),m.group().decode())
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'));print('GUID',uuid.UUID(bytes_le=p.get_data(0x1cbbc48,16)))
