from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,struct,json
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);p.parse_data_directories(directories=[3]);cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);base=p.OPTIONAL_HEADER.ImageBase;rows=[]
for table in [0x8d3d0,0x8e450]:
 f=struct.unpack('<Q',p.get_data(table+0xbf*8,8))[0]-base
 bounds=next(((e.struct.BeginAddress,e.struct.EndAddress)for e in p.DIRECTORY_ENTRY_EXCEPTION if e.struct.BeginAddress<=f<e.struct.EndAddress),(f,f+128))
 a=[f'{i.address:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(bounds[0],bounds[1]-bounds[0]),base+bounds[0])];rows.append({'table':hex(table),'index':'0xbf','function':hex(f),'asm':a});print(hex(table),hex(f),'\n'+'\n'.join(a))
(r/'analysis/graphics-port/umd-performance-slot-bf.json').write_text(json.dumps(rows,indent=2))
