from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0]);cs.detail=True;cs.skipdata=True
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'),fast_load=True)
for s in p.sections:
 if not s.Characteristics&0x20000000:continue
 from collections import deque
 last=deque(maxlen=8)
 for i in cs.disasm(s.get_data(),0x140000000+s.VirtualAddress):
  if i.id and i.mnemonic=='call'and i.operands[0].type==capstone.CS_OP_IMM and i.operands[0].imm==0x1400f36d0:
   if any(v.mnemonic=='mov'and v.op_str=='edx, 0xf'for v in last):print('\n'.join(f'{v.address-0x140000000:x}: {v.mnemonic} {v.op_str}'for v in list(last)+[i]))
  last.append(i)
