from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0])
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'),fast_load=True);p.parse_data_directories(directories=[3]);fs=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in p.DIRECTORY_ENTRY_EXCEPTION);a,z=next((a,z)for a,z in fs if a<=0xf40a9<z);print(hex(a),hex(z));print('\n'.join(f'{i.address-0x140000000:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(a,z-a),0x140000000+a)))
