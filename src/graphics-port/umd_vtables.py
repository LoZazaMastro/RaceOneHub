from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re,struct,json
r=project_root(__file__);add_python_libraries(__file__);import pefile
p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll';pe=pefile.PE(str(p),fast_load=True);base=pe.OPTIONAL_HEADER.ImageBase;out=[]
for f in [0x6ae8,0xb150,0x10ba0,0x11018]:
 for m in re.finditer(re.escape(struct.pack('<Q',base+f)),pe.__data__):
  rva=pe.get_rva_from_offset(m.start()); row={'function':hex(f),'pointer_rva':hex(rva),'first16':[hex(x-base)for x in struct.unpack('<16Q',pe.get_data(rva,128))]};out.append(row);print(row)
(r/'analysis/graphics-port/umd-context-vtables.json').write_text(json.dumps(out,indent=2))
