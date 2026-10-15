from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0]);cs.detail=True;cs.skipdata=True;import re,struct
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'),fast_load=True)
for target in [0x4f730,0x4f7c0,0x4eed0]:
 print('TARGET',hex(target),'QWORDsites',[hex(p.get_rva_from_offset(m.start()))for m in re.finditer(re.escape(struct.pack('<Q',0x140000000+target)),p.__data__)])
for s in p.sections:
 if not s.Characteristics&0x20000000:continue
 from collections import deque
 last=deque(maxlen=5)
 for i in cs.disasm(s.get_data(),0x140000000+s.VirtualAddress):
  if i.id and any(o.type==capstone.CS_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP and i.address+i.size+o.mem.disp in [0x14004f730,0x14004f7c0,0x14004eed0]for o in i.operands):print('\n'.join(f'{v.address-0x140000000:x}: {v.mnemonic} {v.op_str}'for v in list(last)+[i]))
  last.append(i)
