from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,struct,bisect,json
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);pe.parse_data_directories(directories=[3]);rgs=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in pe.DIRECTORY_ENTRY_EXCEPTION);starts=[a for a,z in rgs];cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);base=pe.OPTIONAL_HEADER.ImageBase;out=[]
for table in [0x8d3d0,0x8e450]:
 for idx in [0x29,0x2a]:
  f=struct.unpack('<Q',pe.get_data(table+idx*8,8))[0]-base;a,z=rgs[bisect.bisect_right(starts,f)-1];a,z=(a,z)if a<=f<z else(f,f+128)
  asm=[f'{i.address:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(pe.get_data(a,z-a),base+a)];print(hex(table),hex(idx),hex(f));print('\n'.join(asm));out.append({'table':hex(table),'slot':hex(idx),'function':hex(f),'asm':asm})
(r/'analysis/graphics-port/umd-dispatch-methods.json').write_text(json.dumps(out,indent=2))
