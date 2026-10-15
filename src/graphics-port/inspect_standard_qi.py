from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,bisect
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
for mod,funcs in [('umd.dll',[0x11018,0x16e3c,0x16e98,0x16f00,0x10ba0]),('d3d11.dll',[0x11088,0x33fc0])]:
 p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/mod),fast_load=True);p.parse_data_directories(directories=[3]);rs=sorted((x.struct.BeginAddress,x.struct.EndAddress)for x in p.DIRECTORY_ENTRY_EXCEPTION);st=[a for a,z in rs];b=p.OPTIONAL_HEADER.ImageBase
 for f in funcs:
  a,z=rs[bisect.bisect_right(st,f)-1];a,z=(a,z)if a<=f<z else(f,f+256);print('\nMODULE',mod,'FUNCTION',hex(f));print('\n'.join(f'{i.address-b:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(a,z-a),b+a)))
