from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import collections,hashlib,json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
from capstone.x86 import X86_OP_REG,X86_OP_MEM,X86_OP_IMM
OUT=ROOT/'analysis/audio-tls/tls-boundary';source=ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'
p=pefile.PE(str(source));base=p.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
registers=['rax','rcx','rdx','rbx','rsp','rbp','rsi','rdi']+[f'r{i}' for i in range(8,16)]
def canonical(reg):
    n=cs.reg_name(reg) or ''
    aliases={'eax':'rax','ax':'rax','al':'rax','ah':'rax','ebx':'rbx','bx':'rbx','bl':'rbx','bh':'rbx','ecx':'rcx','cx':'rcx','cl':'rcx','ch':'rcx','edx':'rdx','dx':'rdx','dl':'rdx','dh':'rdx','esi':'rsi','edi':'rdi','ebp':'rbp','esp':'rsp'}
    return aliases.get(n,n[:-1] if n.startswith('r') and n[-1:] in ['d','w','b'] else n)
cache={}
def instruction(rva):
    if rva not in cache:cache[rva]=next(cs.disasm(p.get_data(rva,16),base+rva),None)
    return cache[rva]
rows=[]
for site in json.loads((ROOT/'analysis/pc-build/guest-tls-vector-sites.json').read_text()):
    rva=int(site['rva'],16);first=instruction(rva);destination=canonical(first.operands[0].reg)
    queue=[(rva+first.size,frozenset([destination]))];visited=set();uses={};trace={};reason=None
    if destination=='rsp':reason='stack_pointer_destination'
    while queue and reason is None:
        pc,live=queue.pop()
        if not live or (pc,live) in visited:continue
        visited.add((pc,live))
        if len(visited)>2048:reason='cfg_budget';break
        ins=instruction(pc)
        if not ins:reason='invalid_instruction';break
        trace[pc]=f'{pc:x}: {ins.mnemonic} {ins.op_str}'
        aliases=set(live);copy=None
        if ins.mnemonic=='nop':queue.append((pc+ins.size,live));continue
        if ins.mnemonic=='int3':reason='padding_before_dead';break
        if ins.mnemonic=='ret':
            if 'rax' in aliases:reason='return_register_live'
            continue
        if ins.mnemonic=='call':
            if aliases&{'rcx','rdx','r8','r9'}:
                if not (rva==0x7780ed and pc==0x778134 and aliases=={'rcx'}):reason='call_argument_live';break
                # Exact callee 7e1d20 overwrites RCX at7e1d32 before any read.
                if ins.operands[0].type!=X86_OP_IMM or ins.operands[0].imm-base!=0x7e1d20:reason='call_target_changed';break
            if any((op.type==X86_OP_REG and canonical(op.reg) in aliases) or (op.type==X86_OP_MEM and (canonical(op.mem.base) in aliases or canonical(op.mem.index) in aliases)) for op in ins.operands):reason='vector_used_as_call_target';break
            aliases-={'rax','rcx','rdx','r8','r9','r10','r11'};queue.append((pc+ins.size,frozenset(aliases)));continue
        if ins.group(capstone.CS_GRP_JUMP):
            target=ins.operands[0]
            if target.type!=X86_OP_IMM:reason='indirect_jump_before_dead';break
            queue.append((target.imm-base,live))
            if ins.mnemonic!='jmp':queue.append((pc+ins.size,live))
            continue
        zeroing=ins.mnemonic=='xor' and len(ins.operands)==2 and all(op.type==X86_OP_REG for op in ins.operands) and ins.operands[0].reg==ins.operands[1].reg
        if ins.mnemonic in ['mov','lea'] and len(ins.operands)==2 and ins.operands[0].type==X86_OP_REG:
            src=ins.operands[1]
            if ins.mnemonic=='mov' and src.type==X86_OP_REG and canonical(src.reg) in aliases:
                if src.size!=8:reason='truncated_vector_copy';break
                copy=canonical(ins.operands[0].reg)
            if ins.mnemonic=='lea' and src.type==X86_OP_MEM and canonical(src.mem.base) in aliases:
                if src.mem.index or src.mem.disp:reason='vector_address_arithmetic';break
                copy=canonical(ins.operands[0].reg)
        for op in ins.operands:
            if op.type==X86_OP_MEM:
                if canonical(op.mem.index) in aliases:reason='vector_used_as_index';break
                if canonical(op.mem.base) in aliases and ins.mnemonic!='lea':
                    if op.mem.index or op.mem.disp:reason='nonzero_or_indexed_lookup';break
                    if op.size!=8 or not op.access&capstone.CS_AC_READ or op.access&capstone.CS_AC_WRITE:reason='nonpointer_vector_lookup';break
                    uses[pc]=trace[pc]
            elif op.type==X86_OP_REG and canonical(op.reg) in aliases and op.access&capstone.CS_AC_READ and copy is None and not zeroing:
                reason='vector_value_escape';break
        if reason:break
        _,written=ins.regs_access()
        aliases-={canonical(op.reg) for op in ins.operands if op.type==X86_OP_REG and op.access&capstone.CS_AC_WRITE and op.size>=4}
        if copy:aliases.add(copy)
        queue.append((pc+ins.size,frozenset(aliases)))
    if not reason and not uses:reason='no_vector0_lookup'
    if rva in [0xa1c104,0xa3bab3] and reason=='return_register_live':reason='exact_preserving_double_load'
    rows.append({'rva':hex(rva),'destination':destination,'bytes':first.bytes.hex(),'classification':reason or 'safe_vector0_only','fixed_zero_uses':list(uses.values()),'cfg_states':len(visited),'trace':[trace[a] for a in sorted(trace)]})
summary={'game_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'total_sites':len(rows),'classification_counts':dict(collections.Counter(x['classification'] for x in rows)),'destination_counts':dict(collections.Counter(x['destination'] for x in rows))}
(OUT/'guest-vector-cfg-proof.json').write_text(json.dumps({**summary,'sites':rows},indent=2))
safe=[row for row in rows if row['classification'] in ['safe_vector0_only','exact_preserving_double_load']]
if len(rows)!=327 or len(safe)!=327:raise RuntimeError('Incomplete classification; no partial recipe emitted')
# Re-read every runtime function independently so no linear decode crosses data/padding.
branch_targets=set()
for fn in p.DIRECTORY_ENTRY_EXCEPTION:
    a,b=fn.struct.BeginAddress,fn.struct.EndAddress
    for ins in cs.disasm(p.get_data(a,b-a),base+a):
        if (ins.group(capstone.CS_GRP_JUMP) or ins.mnemonic=='call') and ins.operands and ins.operands[0].type==X86_OP_IMM:branch_targets.add(ins.operands[0].imm-base)
for row in safe:
    special=row['classification']=='exact_preserving_double_load'
    row['patch_bytes']=12 if special else 9
    row['lookup_destination']= (1 if int(row['rva'],16)==0xa1c104 else 2) if special else 255
    if special:
        a=int(row['rva'],16);n=instruction(a+9)
        if n.size!=3 or n.mnemonic!='mov' or n.operands[0].type!=X86_OP_REG or canonical(n.operands[0].reg)!=registers[row['lookup_destination']] or n.operands[1].type!=X86_OP_MEM or canonical(n.operands[1].mem.base)!='rax' or n.operands[1].mem.disp or n.operands[1].mem.index:raise RuntimeError('Special lookup changed')
        if any(a<t<a+12 for t in branch_targets):raise RuntimeError('Internal branch target')
        row['bytes']=p.get_data(a,12).hex()
        row['internal_branch_targets']=[]
summary['classification_counts']=dict(collections.Counter(x['classification'] for x in rows))
summary['interprocedural_override']={'site':'0x7780ed','call':'0x778134','callee':'0x7e1d20','proof':'callee overwrites RCX at7e1d32 before any read; no incoming RCX preserved'}
(OUT/'guest-vector-cfg-proof.json').write_text(json.dumps({**summary,'sites':rows},indent=2))
header=['#pragma once','#include <stdint.h>','typedef struct FM5TLSProofSite {uint32_t rva;uint8_t destination;uint8_t patch_bytes;uint8_t lookup_destination;uint8_t expected[12];} FM5TLSProofSite;',f'#define FM5_TLS_GUEST_SITE_COUNT {len(safe)}u','#define FM5_TLS_GUEST_ALL_SITES_CLASSIFIED 1u','#define FM5_TLS_GUEST_INDEX_RVA 0x261d1b8u','static const FM5TLSProofSite fm5_tls_guest_sites[FM5_TLS_GUEST_SITE_COUNT]={']
for row in safe:header.append('    {0x%su,%u,%u,%u,{%s}},'%(row['rva'][2:],registers.index(row['destination']),row['patch_bytes'],row['lookup_destination'],','.join('0x'+row['bytes'][i:i+2] for i in range(0,len(row['bytes']),2))))
header.append('};');(Path(__file__).parent/'guest_tls_sites.h').write_text('\n'.join(header)+'\n')
print(json.dumps(summary,indent=2));print('RECIPE',hashlib.sha256((Path(__file__).parent/'guest_tls_sites.h').read_bytes()).hexdigest())
