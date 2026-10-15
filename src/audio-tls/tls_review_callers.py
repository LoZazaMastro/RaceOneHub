from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/audio-tls/tls_review.py').read_text().split('for q in [')[0])
targets=[0xa1c0f0,0xa3ba90];out={hex(t):{'refs':[],'pointers':[]} for t in targets}
for sec in p.sections:
 if not sec.Characteristics&0x20000000:continue
 raw=sec.get_data();sr=sec.VirtualAddress
 for at in range(len(raw)-7):
  va=sr+at
  if raw[at] in [0xe8,0xe9] and va+5+int.from_bytes(raw[at+1:at+5],'little',signed=True) in targets:
   t=va+5+int.from_bytes(raw[at+1:at+5],'little',signed=True);ins=next(c.disasm(raw[at:at+15],base+va));out[hex(t)]['refs'].append({'rva':hex(va),'type':ins.mnemonic,'after':[f'{j.address-base:x}: {j.mnemonic} {j.op_str}' for j in c.disasm(raw[at+ins.size:at+ins.size+90],base+va+ins.size)]})
  if raw[at] in [0x48,0x4c] and raw[at+1]==0x8d and raw[at+2]&0xc7==5:
   t=va+7+int.from_bytes(raw[at+3:at+7],'little',signed=True)
   if t in targets:out[hex(t)]['refs'].append({'rva':hex(va),'type':'lea','after':[f'{j.address-base:x}: {j.mnemonic} {j.op_str}' for j in c.disasm(raw[at:at+85],base+va)]})
raw=(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe').read_bytes()
for t in targets:
 pat=(base+t).to_bytes(8,'little');at=raw.find(pat)
 while at>=0:out[hex(t)]['pointers'].append(hex(p.get_rva_from_offset(at)));at=raw.find(pat,at+1)
(r/'analysis/audio-tls/tls-boundary/review-callers.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
