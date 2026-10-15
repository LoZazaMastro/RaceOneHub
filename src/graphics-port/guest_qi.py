from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,json,uuid
r=project_root(__file__);add_python_libraries(__file__)
import pefile,capstone
pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'));cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
for start,size in [(0x69f110,0x100),(0x6bb200,200)]:
 print('\n'.join(f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in cs.disasm(pe.get_data(start,size),0x140000000+start)))
