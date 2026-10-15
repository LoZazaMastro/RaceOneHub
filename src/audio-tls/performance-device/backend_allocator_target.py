from pathlib import Path
exec((Path(__file__).parent/'audit_constructor.py').read_text().split('targets=')[0])
vbtable=0x822d1+7+0x70c570
vtable=0x82419+7+0x70c448
adjust=struct.unpack('<i',p.get_data(vbtable+0xc,4))[0]
function=struct.unpack('<Q',p.get_data(vtable+0x28,8))[0]-base
i=bisect.bisect_right(starts,function)-1
valid=i>=0 and function<functions[i][1]
a,b=functions[i] if valid else (function,function+320)
assembly=[]
for x in cs.disasm(p.get_data(a,b-a),base+a):
    assembly.append(f'{x.address-base:x}: {x.mnemonic} {x.op_str}')
    if not valid and x.mnemonic in ['ret','jmp']:break
target=0x84924;i=bisect.bisect_right(starts,target)-1
valid=i>=0 and target<functions[i][1]
a,b=functions[i] if valid else (target,target+256)
body=[]
for x in cs.disasm(p.get_data(a,b-a),base+a):
    body.append(f'{x.address-base:x}: {x.mnemonic} {x.op_str}')
    if not valid and x.mnemonic=='ret':break
result={'device_backend_constructor':'0x82298','virtual_base_table':hex(vbtable),'virtual_base_adjustment':hex(adjust),'selected_interface_offset':hex(8+adjust),'interface_vtable':hex(vtable),'slot5_target':hex(function),'assembly':assembly,'target':'0x84924','target_assembly':body}
(OUT/'backend-allocator-target.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
