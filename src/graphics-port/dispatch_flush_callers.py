from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,struct,bisect,re,json
r=project_root(__file__);add_python_libraries(__file__);import pefile
pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);pe.parse_data_directories(directories=[3]);rgs=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in pe.DIRECTORY_ENTRY_EXCEPTION);starts=[a for a,z in rgs];b=pe.sections[0].get_data();refs=[]
for m in re.finditer(b'\xe8',b):
 k=m.start()
 if k+5>len(b):continue
 a=pe.sections[0].VirtualAddress+k
 if a+5+struct.unpack_from('<i',b,k+1)[0]!=0x3aac:continue
 f=rgs[bisect.bisect_right(starts,a)-1][0];base=pe.OPTIONAL_HEADER.ImageBase;table=struct.unpack('<202Q',pe.get_data(0x8d3d0,202*8));ind=[hex(i)for i,v in enumerate(table)if v-base==f];row={'call':hex(a),'function':hex(f),'vtable_indices':ind};refs.append(row);print(row)
(r/'analysis/graphics-port/umd-post-dispatch-flush-callers.json').write_text(json.dumps(refs,indent=2))
