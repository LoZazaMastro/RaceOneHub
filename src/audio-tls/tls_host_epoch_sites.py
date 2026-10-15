from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
from capstone.x86 import X86_OP_REG,X86_OP_MEM,X86_OP_IMM
pe=pefile.PE(str(ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe'));base=pe.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
def name(reg):
    n=cs.reg_name(reg) or ''
    aliases={'eax':'rax','ebx':'rbx','ecx':'rcx','edx':'rdx','esi':'rsi','edi':'rdi','ebp':'rbp','esp':'rsp'}
    return aliases.get(n,n[:-1] if n.startswith('r') and n.endswith('d') else n)
rows=[]
for site in json.loads((ROOT/'analysis/audio-tls/tls-boundary/host-accesses.json').read_text()):
    rva=int(site['rva'],16);start=int(site['function'],16)
    instructions=list(cs.disasm(pe.get_data(start,rva-start+200),base+start))
    position=next((i for i,x in enumerate(instructions) if x.address==base+rva),None)
    if position is None:continue
    instructions=instructions[max(0,position-10):position+26]
    vector=set();tls={};constants={};access=[];assembly=[];indexes=set()
    for x in instructions:
        assembly.append(f'{x.address-base:x}: {x.mnemonic} {x.op_str}')
        if x.mnemonic in ['ret','jmp']:break
        for op in x.operands:
            if op.type!=X86_OP_MEM:continue
            m=op.mem;b=name(m.base);idx=name(m.index) if m.index else None
            field=tls.get(b);origin=None
            if field is not None and (not idx or idx in constants):
                if idx:field+=constants[idx][0]*m.scale;origin=constants[idx][1]
                field+=m.disp
            elif idx in tls and b in constants:field=tls[idx]+m.disp+constants[b][0];origin=constants[b][1]
            else:field=None
            if field is not None:access.append({'rva':hex(x.address-base),'offset':field,'bytes':op.size,'write':bool(op.access&capstone.CS_AC_WRITE),'index_immediate_rva':origin,'instruction':assembly[-1]})
        if not x.operands:continue
        if x.mnemonic=='call':
            if x.address>=base+rva:break
            for volatile in ['rax','rcx','rdx','r8','r9','r10','r11']:constants.pop(volatile,None);tls.pop(volatile,None)
            continue
        d=x.operands[0]
        if d.type!=X86_OP_REG:continue
        dst=name(d.reg);src=x.operands[1] if len(x.operands)>1 else None
        old=(tls.get(dst),constants.get(dst))
        if x.address==base+rva:vector.add(dst);continue
        if x.mnemonic in ['mov','movzx','movsxd','movsx']:
            tls.pop(dst,None);constants.pop(dst,None)
            if src.type==X86_OP_IMM:constants[dst]=(src.imm,hex(x.address-base))
            elif src.type==X86_OP_REG:
                n=name(src.reg)
                if n in tls:tls[dst]=tls[n]
                elif n in constants:constants[dst]=constants[n]
            elif src.type==X86_OP_MEM:
                m=src.mem
                if name(m.base) in vector and m.disp==0 and (not m.index or name(m.index) in indexes):tls[dst]=0
                if m.base==capstone.x86.X86_REG_RIP and x.address+x.size+m.disp-base==0x329084:indexes.add(dst)
        elif x.mnemonic=='lea' and src.type==X86_OP_MEM and name(src.mem.base) in tls and not src.mem.index:tls[dst]=tls[name(src.mem.base)]+src.mem.disp
        elif x.mnemonic=='add':
            if src.type==X86_OP_REG and name(src.reg) in tls and old[1] is not None:tls[dst]=tls[name(src.reg)]+old[1][0]
            elif src.type==X86_OP_MEM and name(src.mem.base) in vector and src.mem.disp==0 and not src.mem.index and old[1] is not None:tls[dst]=old[1][0]
            elif src.type==X86_OP_IMM and old[0] is not None:tls[dst]=old[0]+src.imm
        elif x.mnemonic=='xor' and src and src.type==X86_OP_REG and name(src.reg)==dst:constants[dst]=(0,hex(x.address-base));tls.pop(dst,None)
    matched=[a for a in access if a['offset']<40]
    if matched:rows.append({'site':site['rva'],'function':site['function'],'accesses':matched,'assembly':assembly})
(ROOT/'analysis/audio-tls/tls-boundary/host-low40-accesses.json').write_text(json.dumps(rows,indent=2))
epoch=[a for row in rows for a in row['accesses'] if a['offset']==16]
print(json.dumps({'matched_low40_sites':len(rows),'epoch_accesses':len(epoch),'epoch_immediate_rvas':sorted(set(a['index_immediate_rva'] for a in epoch if a['index_immediate_rva'])),'offsets':sorted(set(a['offset'] for row in rows for a in row['accesses']))},indent=2))
