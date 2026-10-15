from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,json
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True;base=pe.OPTIONAL_HEADER.ImageBase;ins=list(cs.disasm(pe.sections[0].get_data(),base+pe.sections[0].VirtualAddress));out=[]
for k,i in enumerate(ins):
 if any(op.type==capstone.CS_OP_MEM and op.mem.disp==0x678 for op in i.operands):
  a=[f'{x.address:x}: {x.mnemonic} {x.op_str}'for x in ins[max(0,k-3):k+7]];out.append({'site':hex(i.address),'asm':a});print('\n'.join(a))
(r/'analysis/graphics-port/umd-context-state678-refs.json').write_text(json.dumps(out,indent=2))
