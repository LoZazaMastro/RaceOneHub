from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,json,struct,uuid,re
r=project_root(__file__);add_python_libraries(__file__)
import pefile,capstone
p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/d3d11.dll';pe=pefile.PE(str(p));cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
out={'exports':[], 'strings':[]}
for x in pe.DIRECTORY_ENTRY_EXPORT.symbols:
 name=x.name.decode() if x.name else None
 out['exports'].append({'name':name,'rva':hex(x.address)})
print(out['exports'])
for m in re.finditer(rb'[\x20-\x7e]{5,}',pe.__data__):
 t=m.group().decode()
 if any(s in t for s in ['Fence','WriteValue','Insert','WaitFor','WaitOn']):out['strings'].append({'rva':hex(pe.get_rva_from_offset(m.start())),'text':t})
print('strings',out['strings'][:50]);(r/'analysis/graphics-port/fm5-native-driver-inventory.json').write_text(json.dumps(out,indent=2))
