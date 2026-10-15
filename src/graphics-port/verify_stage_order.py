from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'),fast_load=True);cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
for a in [0xf59a0,0xf5a30,0xf5a60]:print('\n'.join(f'{i.address:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(a,30),0x140000000+a)))
