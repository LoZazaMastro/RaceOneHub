from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,json,bisect
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll';pe=pefile.PE(str(p),fast_load=True);pe.parse_data_directories(directories=[3]);cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True;sec=pe.sections[0];base=pe.OPTIONAL_HEADER.ImageBase;ranges=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in pe.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0]for x in ranges];out=[]
for i in cs.disasm(sec.get_data(),base+sec.VirtualAddress):
 for op in i.operands:
  if op.type==capstone.CS_OP_MEM and op.mem.base==capstone.x86.X86_REG_RIP and i.address+i.size+op.mem.disp==base+0x7640e8:
   start,end=ranges[bisect.bisect_right(starts,i.address-base)-1];print('REF',hex(i.address),'FUNC',hex(start));asm=[f'{j.address:x}: {j.mnemonic} {j.op_str}' for j in cs.disasm(pe.get_data(start,end-start),base+start)];print('\n'.join(asm));out.append({'ref':hex(i.address),'function':hex(start),'asm':asm})
(r/'analysis/graphics-port/umd-performance-references.json').write_text(json.dumps(out,indent=2))
