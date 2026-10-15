from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_root, project_root
import json,sys,struct,uuid
r=project_root(__file__);add_python_libraries(__file__)
import pefile,capstone
s=r/'analysis/rea/session'
for n,a in [('301-graphics-object-ctor','0x1400d99e0'),('302-graphics-context-ctor','0x1400f47a0')]:
 (s/(n+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':a}}))
ps=[r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/d3d11_x.dll',external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/d3d11_x.dll']
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
out=[]
for p in ps:
 pe=pefile.PE(str(p));v={'path':str(p),'iids':{},'factory_asm':[]}
 for x in pe.DIRECTORY_ENTRY_EXPORT.symbols:
  if x.name and x.name.decode() in ['IID_ID3D11Device','IID_ID3D11Device1','IID_ID3D11DeviceX','IID_ID3D11DeviceContext','IID_ID3D11DeviceContext1','IID_ID3D11DeviceContextX']:
   v['iids'][x.name.decode()]=str(uuid.UUID(bytes_le=pe.get_data(x.address,16)))
 for start,size in [(0x11a4,0x210),(0x13b4,0x180)] if p==ps[0] else [(0x149c,44)]:
  v['factory_asm'] += [f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in cs.disasm(pe.get_data(start,size),pe.OPTIONAL_HEADER.ImageBase+start)]
 out.append(v)
(r/'analysis/graphics-port/factory-abi.json').write_text(json.dumps(out,indent=2))
print(json.dumps(out,indent=2))
