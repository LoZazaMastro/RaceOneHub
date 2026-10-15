from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,bisect,json
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);pe.parse_data_directories(directories=[3]);cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);base=pe.OPTIONAL_HEADER.ImageBase;ranges=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in pe.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0]for x in ranges];out=[]
for f in [0x3ad1,0x649b0]:
 a,z=ranges[bisect.bisect_right(starts,f)-1];ss=[f'{i.address:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(pe.get_data(a,z-a),base+a)];print(hex(f),'func',hex(a));print('\n'.join(ss));out.append({'function':hex(a),'asm':ss})
(r/'analysis/graphics-port/umd-automatic-cache-flush-candidate.json').write_text(json.dumps(out,indent=2))
