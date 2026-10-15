from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect,json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
from capstone.x86 import X86_OP_MEM,X86_REG_RIP
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True;cs.skipdata=True
OUT=ROOT/'analysis/audio-tls/performance-device';out=[]
for name,target in [('d3d11.dll',0x118a20),('d3d11i.dll',0x15f9d0),('umd.dll',0x8ed70)]:
    p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/name))
    base=p.OPTIONAL_HEADER.ImageBase;funcs=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in p.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0] for x in funcs];seen=set()
    for s in p.sections:
        if not s.Characteristics&0x20000000:continue
        for ins in cs.disasm(s.get_data(),base+s.VirtualAddress):
            if ins.id==0:continue
            if not any(o.type==X86_OP_MEM and o.mem.base==X86_REG_RIP and ins.address+ins.size+o.mem.disp-base in [target,target+8] for o in ins.operands):continue
            rva=ins.address-base;i=bisect.bisect_right(starts,rva)-1
            if i<0 or rva>=funcs[i][1]:continue
            a,b=funcs[i]
            if a in seen:continue
            seen.add(a);out.append({'module':name,'ref_rva':hex(rva),'procedure':hex(a),'assembly':[f'{x.address-base:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(p.get_data(a,b-a),base+a)]})
(OUT/'driver-qi-procedures.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
