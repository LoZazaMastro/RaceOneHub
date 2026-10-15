from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,json
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll';pe=pefile.PE(str(p),fast_load=True);cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True;sec=pe.sections[0];base=pe.OPTIONAL_HEADER.ImageBase;ins=list(cs.disasm(sec.get_data(),base+sec.VirtualAddress));out=[]
for k,i in enumerate(ins):
 if i.operands and i.operands[0].type==capstone.CS_OP_MEM and i.operands[0].mem.disp==0x618:
  a=[f'{x.address:x}: {x.mnemonic} {x.op_str}' for x in ins[max(0,k-10):k+4]];out.append({'site':hex(i.address),'asm':a});print('\n'.join(a))
(r/'analysis/graphics-port/umd-inline-618-writes.json').write_text(json.dumps(out,indent=2))
