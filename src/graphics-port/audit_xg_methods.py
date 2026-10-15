from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_game
import sys,json,re,difflib
r=Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile,capstone
paths={
 'old':r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/xg_x.dll',
 'new':external_fh2_game() / 'EmbeddedXvd/Windows/System32/xg_x.dll'}
entries={'old':[0x12c0,0x12d0,0x1310,0x2500,0x2540,0x2610],
         'new':[0x13b0,0x13c0,0x1400,0x2a30,0x2a70,0x2b40]}
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
all_methods={};summary=[]
for name,path in paths.items():
 p=pefile.PE(str(path),fast_load=True);p.parse_data_directories(directories=[3]);base=p.OPTIONAL_HEADER.ImageBase
 methods=[]
 for slot,a in enumerate(entries[name]):
  runtime=next((x.struct for x in p.DIRECTORY_ENTRY_EXCEPTION if x.struct.BeginAddress<=a<x.struct.EndAddress),None)
  if runtime:end=runtime.EndAddress
  else:
   seq=list(cs.disasm(p.get_data(a,256),base+a));last=next((x for x in seq if x.mnemonic=='ret'),seq[-1]);end=last.address-base+last.size
  # Method3 is a leaf with multiple returns. Method5 has chained unwind
  # fragments, so its first RUNTIME_FUNCTION end is not its complete extent.
  if slot==3:end=entries[name][4]
  if slot==5:end=0x2a70 if name=='old' else 0x2fa0
  seq=list(cs.disasm(p.get_data(a,end-a),base+a));index={x.address:i for i,x in enumerate(seq)}
  normalized=[];calls=[]
  for x in seq:
   operands=x.op_str
   if x.mnemonic in ['call','jmp'] or x.mnemonic.startswith('j'):
    if x.operands and x.operands[0].type==capstone.CS_OP_IMM:
     target=x.operands[0].imm
     if target in index:operands=f'local_instruction_{index[target]}'
     elif target-base in entries[name]:operands=f'method_{entries[name].index(target-base)}'
     else:operands='external_target';calls.append(hex(target-base))
   normalized.append(f'{x.mnemonic} {operands}')
  text='\n'.join(f'{x.address-base:x}: {x.mnemonic} {x.op_str}'for x in seq)
  (r/f'analysis/graphics-port/xg-{name}-method{slot}.asm.txt').write_text(text)
  methods.append({'entry':hex(a),'bytes':end-a,'instructions':len(seq),'normalized':normalized,'calls':calls})
 all_methods[name]=methods
for slot in range(6):
 old,new=all_methods['old'][slot],all_methods['new'][slot]
 diff=list(difflib.unified_diff(old['normalized'],new['normalized'],fromfile='old',tofile='new'))
 summary.append({'slot':slot,'old_entry':old['entry'],'new_entry':new['entry'],'old_bytes':old['bytes'],'new_bytes':new['bytes'],'normalized_equal':not diff,'diff':diff,'old_calls':old['calls'],'new_calls':new['calls']})
(r/'analysis/graphics-port/xg-method-audit.json').write_text(json.dumps(summary,indent=2))
for row in summary:
 print(json.dumps(row,indent=2))

# Independently compare the actual offset helpers called by methods4/5.
helpers={}
for name,path in paths.items():
 p=pefile.PE(str(path),fast_load=True);p.parse_data_directories(directories=[3,1]);base=p.OPTIONAL_HEADER.ImageBase
 entries2=[0x9b20,0x6fb0,0xce90] if name=='old' else [0xb440,0x88d0,0xe5d0]
 values=[]
 for a in entries2:
  runtime=next((x.struct for x in p.DIRECTORY_ENTRY_EXCEPTION if x.struct.BeginAddress<=a<x.struct.EndAddress),None)
  end=runtime.EndAddress if runtime else a+16
  # Include chained unwind fragments belonging to this same entry point.
  for fragment in p.DIRECTORY_ENTRY_EXCEPTION:
   if fragment.struct.BeginAddress==end and getattr(fragment.unwindinfo,'FunctionEntry',None)==a:
    end=fragment.struct.EndAddress
  seq=list(cs.disasm(p.get_data(a,end-a),base+a));indices={x.address:i for i,x in enumerate(seq)};norm=[]
  for x in seq:
   op=x.op_str
   if x.operands and x.operands[0].type==capstone.CS_OP_IMM and (x.mnemonic.startswith('j') or x.mnemonic=='call'):
    t=x.operands[0].imm
    op=f'local_{indices[t]}' if t in indices else 'external_target'
   norm.append(f'{x.mnemonic} {op}')
  (r/f'analysis/graphics-port/xg-{name}-helper{a:x}.asm.txt').write_text('\n'.join(f'{x.address-base:x}: {x.mnemonic} {x.op_str}'for x in seq))
  values.append({'rva':hex(a),'bytes':end-a,'normalized':norm})
 helpers[name]=values
result=[]
for i in range(3):
 x,y=helpers['old'][i],helpers['new'][i]
 diff=list(difflib.unified_diff(x['normalized'],y['normalized'],fromfile='old',tofile='new'))
 result.append({'old_rva':x['rva'],'new_rva':y['rva'],'old_bytes':x['bytes'],'new_bytes':y['bytes'],'normalized_equal':not diff,'diff':diff})
(r/'analysis/graphics-port/xg-helper-audit.json').write_text(json.dumps(result,indent=2))
print('Helpers:',json.dumps(result,indent=2))
