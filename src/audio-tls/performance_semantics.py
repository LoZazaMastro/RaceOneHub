from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect,json,struct,sys,uuid
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile,capstone
OUT=ROOT/'analysis/audio-tls/performance-device';p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'));base=p.OPTIONAL_HEADER.ImageBase
fs=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in p.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0] for x in fs];cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
obj=struct.unpack('<14Q',p.get_data(0xc6ce80,112));out=[]
for target in [0x19a64,0x20b04,0x58c48,0x58eb4,0x15654,0x156d8,0x1575c,0x157f8]+[x-base for x in obj]:
    i=bisect.bisect_right(starts,target)-1
    if i>=0 and target<fs[i][1]:a,b=fs[i]
    else:a,b=target,target+1024
    asm=[]
    for x in cs.disasm(p.get_data(a,b-a),base+a):
        asm.append(f'{x.address-base:x}: {x.mnemonic} {x.op_str}')
        if i<0 or target>=fs[i][1]:
            if x.mnemonic=='ret':break
    out.append({'target':hex(target),'assembly':asm})
(OUT/'slot55-object-semantics.json').write_text(json.dumps({'object_vtable':'0xc6ce80','methods':[hex(x-base) for x in obj],'procedures':out},indent=2))
for row in out:print(row['target'],'\n'+'\n'.join(row['assembly']))
