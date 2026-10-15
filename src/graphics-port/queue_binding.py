from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,json
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
s=r/'analysis/rea/session'
for n,a in [('319-graphics-binding-range','0x1400f5e30'),('320-graphics-binding-adjacent','0x1400f5df0')]:
 (s/(n+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':a}}))
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'));cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
a=[f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in cs.disasm(p.get_data(0xf5d70,0x1c0),0x1400f5d70)]
(r/'analysis/graphics-port/binding-setter-assembly.txt').write_text('\n'.join(a));print('\n'.join(a))
