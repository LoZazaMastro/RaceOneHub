from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re,json
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'));sec=next(s for s in pe.sections if s.Name.rstrip(b'\0')==b'.text');data=sec.get_data();base=pe.OPTIONAL_HEADER.ImageBase+sec.VirtualAddress;cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);out=[]
for m in re.finditer(rb'\xff[\x90-\x97]\x48\x02\x00\x00',data):
 rva=sec.VirtualAddress+m.start()
 out.append({'rva':hex(rva),'assembly':[f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in cs.disasm(data[m.start()-80:m.start()+35],base+m.start()-80)]})
print('Total call+248 sites',len(out))
for x in out:
 if 0x690000<=int(x['rva'],16)<0x730000:print(json.dumps(x,indent=2))
(r/'analysis/graphics-port/slot49-calls.json').write_text(json.dumps(out,indent=2))
