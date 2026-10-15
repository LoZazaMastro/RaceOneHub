from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/d3d11_x.dll'));raw=p.__data__
for m in re.finditer(rb'(?:[\x20-\x7e]\x00){4,}',raw):
 s=m.group().decode('utf-16-le')
 if '.dll' in s.lower() or 'd3d' in s.lower():print(hex(p.get_rva_from_offset(m.start())),s)
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);print('\n'.join(f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in cs.disasm(p.get_data(0x10c0,0xe4),0x1800010c0)))
