from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0]);import struct,json
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);cs.detail=True
rows=json.load(open('analysis/graphics-port/context1-sdk-methods.json'));out=[]
for row in rows:
 name=row['name']
 if not any(name==stage+'Set'+kind for stage in ['VS','PS','GS','HS','DS','CS']for kind in ['ConstantBuffers','ShaderResources','Samplers']):continue
 slot=row['old_slot'];f=struct.unpack('<Q',p.get_data(0x8d3d0+8*slot,8))[0]-0x180000000;ins=list(cs.disasm(p.get_data(f,16),0x180000000+f))
 if ins[0].mnemonic=='jmp':f=ins[0].operands[0].imm-0x180000000
 ins=list(cs.disasm(p.get_data(f,800),0x180000000+f));feat=[]
 for i in ins:
  if i.mnemonic=='lea'and any(o.type==capstone.CS_OP_MEM and 0<o.mem.disp<0x1000 for o in i.operands):feat.append(f'{i.address-0x180000000:x}: {i.mnemonic} {i.op_str}')
  if i.mnemonic=='ret':break
 out.append({'name':name,'slot':hex(slot),'function':hex(f),'features':feat});print(name,hex(slot),feat[:8])
(r/'analysis/graphics-port/old-standard-binding-registers.json').write_text(json.dumps(out,indent=2))
