from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
from capstone.x86 import X86_OP_REG,X86_OP_MEM,X86_OP_IMM
pe=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'));base=pe.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
def name(reg):
    n=cs.reg_name(reg) or ''
    aliases={'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','esi':'rsi','edi':'rdi','ebp':'rbp','esp':'rsp'}
    return aliases.get(n,n[:-1] if n.startswith('r') and n.endswith('d') else n)
rows=[]
for site in json.loads((ROOT/'analysis/audio-tls/tls-boundary/game-accesses.json').read_text()):
    rva=int(site['rva'],16);instructions=list(cs.disasm(pe.get_data(rva,240),base+rva))
    vector=set();tls={};constants={};access=[];assembly=[]
    for k,x in enumerate(instructions[:32]):
        assembly.append(f'{x.address-base:x}: {x.mnemonic} {x.op_str}')
        for op in x.operands:
            if op.type!=X86_OP_MEM:continue
            m=op.mem;b=name(m.base);idx=name(m.index) if m.index else None
            offset=tls.get(b)
            if offset is not None and (not idx or idx in constants):offset+=m.disp+(constants[idx]*m.scale if idx else 0)
            elif idx in tls and b in constants:offset=tls[idx]+m.disp+constants[b]
            else:offset=None
            if offset is not None:access.append({'rva':hex(x.address-base),'offset':offset,'bytes':op.size,'write':bool(op.access&capstone.CS_AC_WRITE),'instruction':assembly[-1]})
        if not x.operands:continue
        if x.mnemonic=='call':break
        d=x.operands[0]
        if d.type!=X86_OP_REG:continue
        dst=name(d.reg)
        if k==0:vector.add(dst);continue
        src=x.operands[1] if len(x.operands)>1 else None
        old=(tls.get(dst),constants.get(dst))
        if x.mnemonic in ['mov','movzx','movsxd','movsx']:
            tls.pop(dst,None);constants.pop(dst,None)
            if src.type==X86_OP_IMM:constants[dst]=src.imm
            elif src.type==X86_OP_REG:
                n=name(src.reg)
                if n in tls:tls[dst]=tls[n]
                elif n in constants:constants[dst]=constants[n]
            elif src.type==X86_OP_MEM and name(src.mem.base) in vector and src.mem.disp==0 and not src.mem.index:tls[dst]=0
        elif x.mnemonic=='lea' and src.type==X86_OP_MEM and name(src.mem.base) in tls and not src.mem.index:tls[dst]=tls[name(src.mem.base)]+src.mem.disp
        elif x.mnemonic=='add':
            if src.type==X86_OP_REG and name(src.reg) in tls and old[1] is not None:tls[dst]=tls[name(src.reg)]+old[1]
            elif src.type==X86_OP_MEM and name(src.mem.base) in vector and src.mem.disp==0 and not src.mem.index and old[1] is not None:tls[dst]=old[1]
            elif src.type==X86_OP_IMM and old[0] is not None:tls[dst]=old[0]+src.imm
        elif x.mnemonic=='xor' and src and src.type==X86_OP_REG and name(src.reg)==dst:constants[dst]=0;tls.pop(dst,None)
    matched=[a for a in access if a['offset']<20 and a['offset']+a['bytes']>16]
    if matched:rows.append({'site':site['rva'],'function':site['function'],'accesses':matched,'assembly':assembly})
(ROOT/'analysis/audio-tls/tls-boundary/game-offset10-candidates.json').write_text(json.dumps(rows,indent=2))
print(json.dumps(rows,indent=2))
