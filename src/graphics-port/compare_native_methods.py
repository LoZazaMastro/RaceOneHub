from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_root, project_root
import sys,json,bisect,struct,difflib,collections
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
paths=[r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'];mods=[]
for path,table,n in zip(paths,[0x8d3d0,0xd6950],[200,253]):
 p=pefile.PE(str(path),fast_load=True);p.parse_data_directories(directories=[3]);rs=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in p.DIRECTORY_ENTRY_EXCEPTION);st=[a for a,z in rs];base=p.OPTIONAL_HEADER.ImageBase
 def body(f,depth=0):
  ins=list(cs.disasm(p.get_data(f,16),base+f))
  if ins and ins[0].mnemonic=='jmp' and ins[0].operands[0].type==capstone.CS_OP_IMM and depth<3:return body(ins[0].operands[0].imm-base,depth+1)
  a,z=rs[bisect.bisect_right(st,f)-1];z=z if a<=f<z else f+96
  out=[]
  for i in cs.disasm(p.get_data(f,min(z-f,4000)),base+f):
   ops=[]
   for o in i.operands:
    if o.type==capstone.CS_OP_REG:ops.append(i.reg_name(o.reg))
    elif o.type==capstone.CS_OP_IMM:ops.append('addr' if base<=o.imm<base+p.OPTIONAL_HEADER.SizeOfImage else hex(o.imm))
    elif o.type==capstone.CS_OP_MEM:ops.append('mem' if o.mem.base==capstone.x86.X86_REG_RIP else f'{i.reg_name(o.mem.base)}+{i.reg_name(o.mem.index)}*{o.mem.scale}+DISP')
   out.append(i.mnemonic+':'+','.join(ops))
  return out
 entries=[]
 for j in range(n):
  f=struct.unpack('<Q',p.get_data(table+8*j,8))[0]-base;entries.append((f,body(f)))
 mods.append(entries)
rows=[]
for j,(f,b)in enumerate(mods[0]):
 cand=sorted([(difflib.SequenceMatcher(None,b,c,autojunk=False).ratio(),k,hex(g))for k,(g,c)in enumerate(mods[1])],reverse=True)[:4]
 rows.append({'old_slot':j,'old_function':hex(f),'length':len(b),'candidates':[{'score':round(s,4),'new_slot':k,'new_function':g}for s,k,g in cand]})
(r/'analysis/graphics-port/umd-context-method-similarity.json').write_text(json.dumps(rows,indent=2))
for x in rows:print(f'{x["old_slot"]:02x} {x["old_function"]} {x["length"]}',[(hex(y['new_slot']),y['score'])for y in x['candidates'][:2]])
