from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,json,struct,hashlib
r=project_root(__file__);add_python_libraries(__file__)
import pefile,capstone
s=r/'analysis/rea/session'
for p in s.glob('3*.response.json'):
 j=json.loads(p.read_text()).get('result',{}).get('structuredContent',{}).get('raw_result',{})
 if isinstance(j,dict) and isinstance(j.get('value'),str): (r/'analysis/graphics-port'/p.with_suffix('.c').name).write_text(j['value'])
fm=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'))
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
imports={i.name.decode():i.address for d in fm.DIRECTORY_ENTRY_IMPORT for i in d.imports if i.name and b'D3D11CreateDevice' in i.name}
print(imports)
x=[]
for sec in fm.sections:
 if not sec.Characteristics&0x20000000:continue
 b=sec.get_data();base=fm.OPTIONAL_HEADER.ImageBase+sec.VirtualAddress
 for k in range(len(b)-6):
  if b[k:k+2]!=b'\xff\x15':continue
  target=base+k+6+struct.unpack_from('<i',b,k+2)[0]
  if target in imports.values():
   start=base+k-100
   asm=[f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in cs.disasm(b[k-100:k+250],start)]
   x.append({'site':hex(base+k),'assembly':asm})
print(json.dumps(x,indent=2));(r/'analysis/graphics-port/fm5-factory-calls.json').write_text(json.dumps(x,indent=2))
