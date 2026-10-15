exec((__import__('pathlib').Path(__file__).parent/'audit_constructor.py').read_text().split('targets=')[0])
rows=[]
for section in p.sections:
    if not section.Characteristics & 0x20000000: continue
    for ins in cs.disasm(section.get_data(),base+section.VirtualAddress):
        if not any(op.type==capstone.x86.X86_OP_MEM and op.mem.disp==0x3d278 for op in ins.operands): continue
        target=ins.address-base;i=bisect.bisect_right(starts,target)-1
        a,b=functions[i] if i>=0 and target<functions[i][1] else (target,target+128)
        rows.append({'reference':hex(target),'instruction':f'{ins.mnemonic} {ins.op_str}','function':hex(a),'assembly':[f'{x.address-base:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(p.get_data(a,b-a),base+a)]})
(OUT/'backend-allocator-references.json').write_text(json.dumps(rows,indent=2))
print([(x['reference'],x['instruction'],x['function']) for x in rows])
