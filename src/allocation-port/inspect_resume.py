from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_root
import sys,json,struct,re,bisect
R=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile,capstone
O=R/'analysis/allocation-port/resume';O.mkdir(exist_ok=True)
m=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);m.detail=True;m.skipdata=True
ml=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);ml.skipdata=True
sources=[('guest',R/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe',[0x5a37c0]),('umd',R/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',[0x5ace0,0x5ad78,0x4284,0x10b98]),('fh2',external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',[0x17190,0x171a0,0x94420])]
for label,path,targets in sources:
 p=pefile.PE(str(path));b=p.OPTIONAL_HEADER.ImageBase;funcs=[x.struct for x in p.DIRECTORY_ENTRY_EXCEPTION];starts=[f.BeginAddress for f in funcs]
 def fun(a):
  i=bisect.bisect_right(starts,a)-1
  return funcs[i]if i>=0 and a<funcs[i].EndAddress else None
 for a in targets:
  f=fun(a);end=f.EndAddress if f else a+128
  (O/f'{label}-{a:x}.txt').write_text('\n'.join(f'{x.address-b:x}: {x.mnemonic} {x.op_str}'for x in m.disasm(p.get_data(a,end-a),b+a)))
 if label=='guest':
  c=[]
  text=next(s for s in p.sections if s.Name.startswith(b'.text'))
  for a,z,mn,op in ml.disasm_lite(text.get_data(),b+text.VirtualAddress):
   if mn not in ['call','jmp'] or not any(k in op for k in [' + 0x630]',' + 0x610]',' + 0x628]']):continue
   f=fun(a-b);ins=list(m.disasm(p.get_data(f.BeginAddress,f.EndAddress-f.BeginAddress),b+f.BeginAddress)) if f else []
   ix=next((i for i,x in enumerate(ins)if x.address==a),None)
   c.append({'rva':hex(a-b),'function':hex(f.BeginAddress)if f else None,'instruction':mn+' '+op,'assembly':[f'{x.address-b:x}: {x.mnemonic} {x.op_str}'for x in ins[max(0,ix-14):ix+4]]if ix is not None else []})
  (O/'guest-inline-resume-candidates.json').write_text(json.dumps(c,indent=2));print('guestcandidates',len(c))
  for x in c:print(json.dumps(x))
