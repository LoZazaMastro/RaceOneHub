from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0]);cs.detail=True;cs.skipdata=True;import json
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);p.parse_data_directories(directories=[3]);rs=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in p.DIRECTORY_ENTRY_EXCEPTION);st=[a for a,z in rs];rows=[]
for s in p.sections:
 if not s.Characteristics&0x20000000:continue
 for i in cs.disasm(s.get_data(),0x180000000+s.VirtualAddress):
  if i.id and i.mnemonic=='lea'and any(o.type==capstone.CS_OP_MEM and o.mem.disp in [0x7d7,0x137]for o in i.operands):
   a=i.address-0x180000000;k=bisect.bisect_right(st,a)-1;f,z=rs[k];row={'site':hex(a),'function':hex(f),'instruction':i.mnemonic+' '+i.op_str};rows.append(row);print(row)
(r/'analysis/graphics-port/uav-register-writers.json').write_text(json.dumps(rows,indent=2))
