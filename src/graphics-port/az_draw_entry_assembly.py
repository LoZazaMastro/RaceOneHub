from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0])
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'),fast_load=True)
for f,n in [(0x12c120,90),(0x12be10,90),(0x12b800,90),(0x12cf90,100),(0x12cec0,90)]:print('\nFUNC',hex(f));print('\n'.join(f'{i.address-0x140000000:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(f,n),0x140000000+f)))
