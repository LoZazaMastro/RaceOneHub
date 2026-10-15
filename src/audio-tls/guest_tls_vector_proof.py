from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import collections,hashlib,json,struct,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
from capstone.x86 import X86_OP_REG,X86_OP_MEM
OUT=ROOT/'analysis/audio-tls/tls-boundary'
source=ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'
pe=pefile.PE(str(source));base=pe.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
registers=['rax','rcx','rdx','rbx','rsp','rbp','rsi','rdi']+[f'r{i}' for i in range(8,16)]
def canonical(reg):
    n=cs.reg_name(reg) or ''
    aliases={'eax':'rax','ax':'rax','al':'rax','ah':'rax','ebx':'rbx','bx':'rbx','bl':'rbx','bh':'rbx','ecx':'rcx','cx':'rcx','cl':'rcx','ch':'rcx','edx':'rdx','dx':'rdx','dl':'rdx','dh':'rdx','esi':'rsi','edi':'rdi','ebp':'rbp','esp':'rsp'}
    return aliases.get(n,n[:-1] if n.startswith('r') and n[-1:] in ['d','w','b'] else n)
rows=[]
for site in json.loads((ROOT/'analysis/pc-build/guest-tls-vector-sites.json').read_text()):
    rva=int(site['rva'],16);code=list(cs.disasm(pe.get_data(rva,320),base+rva));first=code[0]
    destination=canonical(first.operands[0].reg)
    row={'rva':hex(rva),'destination':destination,'bytes':first.bytes.hex(),'fixed_zero_uses':[],'trace':[]}
    aliases={destination};reason=None
    if destination=='rsp':reason='stack_pointer_destination'
    for ins in code[1:48]:
        if reason or not aliases:break
        row['trace'].append(f'{ins.address-base:x}: {ins.mnemonic} {ins.op_str}')
        if ins.mnemonic=='ret':aliases.clear();break
        if ins.mnemonic=='int3':reason='padding_before_vector_dead';break
        if ins.group(capstone.CS_GRP_JUMP):reason='branch_before_vector_dead';break
        if ins.mnemonic=='call':
            if aliases&{'rcx','rdx','r8','r9'}:reason='vector_alias_live_in_call_argument';break
            aliases-={'rax','rcx','rdx','r8','r9','r10','r11'}
            continue
        copied=None
        if ins.mnemonic in ['mov','lea'] and len(ins.operands)==2 and ins.operands[0].type==X86_OP_REG:
            src=ins.operands[1]
            if ins.mnemonic=='mov' and src.type==X86_OP_REG and canonical(src.reg) in aliases:
                if src.size!=8:reason='truncated_vector_copy';break
                copied=canonical(ins.operands[0].reg)
            if ins.mnemonic=='lea' and src.type==X86_OP_MEM and canonical(src.mem.base) in aliases:
                if src.mem.index or src.mem.disp:reason='vector_address_arithmetic';break
                copied=canonical(ins.operands[0].reg)
        for index,operand in enumerate(ins.operands):
            if operand.type==X86_OP_MEM:
                m=operand.mem
                if canonical(m.index) in aliases:reason='vector_used_as_index';break
                if canonical(m.base) in aliases and ins.mnemonic!='lea':
                    if m.index or m.disp:reason='nonzero_or_indexed_vector_access';break
                    if not operand.access&capstone.CS_AC_READ or operand.access&capstone.CS_AC_WRITE:reason='vector_store_or_unclassified_access';break
                    row['fixed_zero_uses'].append({'rva':hex(ins.address-base),'instruction':row['trace'][-1]})
            elif operand.type==X86_OP_REG and canonical(operand.reg) in aliases and operand.access&capstone.CS_AC_READ:
                if copied is None:reason='vector_value_escaped_or_arithmetic';break
        if reason:break
        _,writes=ins.regs_access()
        aliases-={canonical(reg) for reg in writes}
        if copied:aliases.add(copied)
    if reason is None and aliases:reason='vector_liveness_unresolved'
    if reason is None and not row['fixed_zero_uses']:reason='no_vector0_lookup'
    row['classification']=reason or 'safe_vector0_only'
    rows.append(row)
result={'game_sha256':hashlib.sha256(source.read_bytes()).hexdigest(),'total_sites':len(rows),'classification_counts':dict(collections.Counter(x['classification'] for x in rows)),'destination_counts':dict(collections.Counter(x['destination'] for x in rows)),'sites':rows}
(OUT/'guest-vector-proof.json').write_text(json.dumps(result,indent=2))
safe=[row for row in rows if row['classification']=='safe_vector0_only']
header=['#pragma once','#include <stdint.h>','typedef struct FM5TLSProofSite {uint32_t rva;uint8_t destination;uint8_t expected[9];} FM5TLSProofSite;',f'#define FM5_TLS_GUEST_SITE_COUNT {len(safe)}u','#define FM5_TLS_GUEST_INDEX_RVA 0x261d1b8u','static const FM5TLSProofSite fm5_tls_guest_sites[FM5_TLS_GUEST_SITE_COUNT]={']
for row in safe:header.append('    {0x%su,%u,{%s}},'%(row['rva'][2:],registers.index(row['destination']),','.join('0x'+row['bytes'][i:i+2] for i in range(0,18,2))))
header.append('};');(Path(__file__).parent/'guest_tls_sites_provisional.h').write_text('\n'.join(header)+'\n')
print(json.dumps({k:v for k,v in result.items() if k!='sites'},indent=2))
print('REVIEW',[(row['rva'],row['classification']) for row in rows if row['classification']!='safe_vector0_only'])

host=pefile.PE(str(ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe'));hostbase=host.OPTIONAL_HEADER.ImageBase
calls=[]
for section in host.sections:
    if not section.Characteristics&0x20000000:continue
    raw=section.get_data()
    for at in range(len(raw)-4):
        if raw[at]!=0xe8:continue
        rva=section.VirtualAddress+at
        if rva+5+struct.unpack_from('<i',raw,at+1)[0]!=0xbe6a0:continue
        calls.append({'call_rva':hex(rva),'bytes':raw[at:at+5].hex(),'return_rva':hex(rva+5),'assembly':[f'{x.address-hostbase:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(host.get_data(rva-40,120),hostbase+rva-40)]})
(OUT/'binder-callers.json').write_text(json.dumps(calls,indent=2));print('BINDERS',json.dumps(calls,indent=2))
