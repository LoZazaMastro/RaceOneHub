from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile,capstone
from capstone.x86 import X86_OP_MEM,X86_REG_RSP,X86_REG_RBP
p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'));base=p.OPTIONAL_HEADER.ImageBase;cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True;out=[]
for f in p.DIRECTORY_ENTRY_EXCEPTION:
    a,b=f.struct.BeginAddress,f.struct.EndAddress
    if not 0x690000<=a<0x6d0000:continue
    ins=list(cs.disasm(p.get_data(a,b-a),base+a))
    hits=[x for x in ins if any(o.type==X86_OP_MEM and o.mem.disp==0x100 and o.mem.base not in [X86_REG_RSP,X86_REG_RBP] for o in x.operands)]
    if hits:out.append({'procedure':hex(a),'hits':[hex(x.address-base) for x in hits],'assembly':[f'{x.address-base:x}: {x.mnemonic} {x.op_str}' for x in ins]})
(ROOT/'analysis/audio-tls/performance-device/scratch-member-candidates.json').write_text(json.dumps(out,indent=2))
for row in out:
    print('FUNCTION',row['procedure'],'HITS',row['hits'])
    for i,line in enumerate(row['assembly']):
        if '[rcx + 0x100]' in line or '[rbx + 0x100]' in line or '[rsi + 0x100]' in line:print('\n'.join(row['assembly'][max(0,i-4):i+8]))
