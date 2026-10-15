from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0]);import struct,json
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);cs.detail=True;rows=[]
for slot in range(0x9b,0xad):
 f=struct.unpack('<Q',p.get_data(0x8d3d0+8*slot,8))[0]-0x180000000;ins=list(cs.disasm(p.get_data(f,250),0x180000000+f));feat=[]
 for i in ins:
  if i.mnemonic in ['lea','shl','add']and any(o.type==capstone.CS_OP_IMM or o.type==capstone.CS_OP_MEM and o.mem.disp for o in i.operands):feat.append(f'{i.address-0x180000000:x}: {i.mnemonic} {i.op_str}')
  if i.mnemonic=='ret':break
 rows.append({'old_slot':hex(slot),'function':hex(f),'register_encoding':feat});print(hex(slot),feat)
(r/'analysis/graphics-port/single-binding-register-proof.json').write_text(json.dumps(rows,indent=2))
