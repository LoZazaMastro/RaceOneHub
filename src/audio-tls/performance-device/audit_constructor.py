from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect,json,struct,sys
ROOT=Path(__file__).resolve().parents[3]
add_python_libraries(__file__)
import pefile,capstone
OUT=ROOT/'analysis/audio-tls/performance-device'
p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'))
base=p.OPTIONAL_HEADER.ImageBase
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
functions=sorted((r.struct.BeginAddress,r.struct.EndAddress) for r in p.DIRECTORY_ENTRY_EXCEPTION)
starts=[r[0] for r in functions]
imports={x.address-base: {'dll':d.dll.decode(),'name':x.name.decode() if x.name else f'ordinal_{x.ordinal}'} for d in p.DIRECTORY_ENTRY_IMPORT for x in d.imports}
targets=[0x19a9c,0x20b04,0x58c48,0x8acb4,0x1575c,0x16e00,0x16d98]
tables=[]
for address in [0x759330,0x759370]:
    entries=struct.unpack('<8Q',p.get_data(address,64))
    tables.append({'rva':hex(address),'kind':'static allocator/type metadata; not a callable vtable','first_eight_qwords_raw':[hex(x) for x in entries]})
    targets.extend(x-base for x in entries if base <= x < base+0x8e000)
procedures=[]
for target in dict.fromkeys(targets):
    i=bisect.bisect_right(starts,target)-1
    if i>=0 and target<functions[i][1]: a,b=functions[i]
    else: a,b=target,target+256
    rows=[]
    for ins in cs.disasm(p.get_data(a,b-a),base+a):
        row={'rva':hex(ins.address-base),'asm':f'{ins.mnemonic} {ins.op_str}'}
        for operand in ins.operands:
            if operand.type==capstone.x86.X86_OP_MEM and operand.mem.base==capstone.x86.X86_REG_RIP:
                ref=ins.address+ins.size+operand.mem.disp-base
                if ref in imports: row['import']=imports[ref]
        rows.append(row)
        if (i<0 or target>=functions[i][1]) and ins.mnemonic=='ret': break
    procedures.append({'target':hex(target),'instructions':rows})
result={'source':'FM5 original umd.dll, static disassembly only','arena_tables':tables,'procedures':procedures}
(OUT/'child-constructor-audit.json').write_text(json.dumps(result,indent=2))
for proc in procedures:
    print(proc['target'])
    for row in proc['instructions']: print(row)
