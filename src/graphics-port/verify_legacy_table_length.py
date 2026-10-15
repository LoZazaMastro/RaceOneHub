from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
s='\n'.join(f'{i.address:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(pe.get_data(0x16a8c,0xdd),0x180016a8c));print(s);(r/'analysis/graphics-port/umd-inline-copy-length.txt').write_text(s)
