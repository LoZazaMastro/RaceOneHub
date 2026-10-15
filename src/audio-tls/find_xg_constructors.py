from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect,json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
p=pefile.PE(str(ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe'));base=p.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True;cs.skipdata=True
functions=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in p.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0] for x in functions]
targets={}
for name in ['XGCreateTexture1DComputer','XGCreateTexture2DComputer','XGCreateTexture3DComputer']:
    b=p.__data__.find(name.encode()+b'\0')
    if b>=0:targets[p.get_rva_from_offset(b)]=name
rows=[]
for section in p.sections:
    if not section.Characteristics&0x20000000:continue
    for ins in cs.disasm(section.get_data(),base+section.VirtualAddress):
        if not ins.id:continue
        for op in ins.operands:
            if op.type!=capstone.x86.X86_OP_MEM or op.mem.base!=capstone.x86.X86_REG_RIP:continue
            ref=ins.address+ins.size+op.mem.disp-base
            if ref not in targets:continue
            a=ins.address-base;i=bisect.bisect_right(starts,a)-1;a,b=functions[i]
            rows.append({'name':targets[ref],'string_rva':hex(ref),'reference':hex(ins.address-base),'function':hex(a),'assembly':[f'{x.address-base:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(p.get_data(a,b-a),base+a)]})
(ROOT/'analysis/audio-tls/xg-port/az-constructor-calls.json').write_text(json.dumps(rows,indent=2))
for row in rows: print(row['name'],row['function'],'\n'+'\n'.join(row['assembly']))
