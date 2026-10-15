from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,struct,sys,bisect
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile,capstone
p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'));base=p.OPTIONAL_HEADER.ImageBase;cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
fs=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in p.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0] for x in fs]
table=struct.unpack('<70Q',p.get_data(0xc6cc70,560));out=[]
for slot,target in enumerate(table):
    target-=base;i=bisect.bisect_right(starts,target)-1
    if i>=0 and target<fs[i][1]:a,b=fs[i]
    else:a,b=target,target+320
    asm=[]
    for x in cs.disasm(p.get_data(a,b-a),base+a):
        asm.append(f'{x.address-base:x}: {x.mnemonic} {x.op_str}')
        if (i<0 or target>=fs[i][1]) and x.mnemonic=='ret':break
    out.append({'slot':slot,'rva':hex(target),'assembly':asm})
(ROOT/'analysis/audio-tls/performance-device/full-authentic-device-table.json').write_text(json.dumps(out,indent=2))
for row in out:print(row['slot'],row['rva'], '\n'+'\n'.join(row['assembly']) if row['slot'] in [3,4,5,6,7,41,54,55] else '')
out=[]
for slot,target in enumerate(struct.unpack('<50Q',p.get_data(0xc6cf70,400))):
    target-=base;i=bisect.bisect_right(starts,target)-1
    if i>=0 and target<fs[i][1]:a,b=fs[i]
    else:a,b=target,target+320
    asm=[]
    for x in cs.disasm(p.get_data(a,b-a),base+a):
        asm.append(f'{x.address-base:x}: {x.mnemonic} {x.op_str}')
        if (i<0 or target>=fs[i][1]) and x.mnemonic=='ret':break
    out.append({'slot':slot,'rva':hex(target),'assembly':asm})
(ROOT/'analysis/audio-tls/performance-device/full-authentic-normal-device-table.json').write_text(json.dumps(out,indent=2))
print('NORMAL',[(x['slot'],x['rva']) for x in out])
