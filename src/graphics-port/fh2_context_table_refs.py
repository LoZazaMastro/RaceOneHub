from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import external_fh2_root, project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0]);cs.detail=True;cs.skipdata=True
p=pefile.PE(str(external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);p.parse_data_directories(directories=[3]);rs=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in p.DIRECTORY_ENTRY_EXCEPTION);st=[a for a,z in rs];b=p.OPTIONAL_HEADER.ImageBase
for s in p.sections:
 if not s.Characteristics&0x20000000:continue
 ins=list(cs.disasm(s.get_data(),b+s.VirtualAddress))
 for j,i in enumerate(ins):
  if i.id and any(o.type==capstone.CS_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP and i.address+i.size+o.mem.disp in [b+0xd6950,b+0x117b60]for o in i.operands):
   print('\nTARGET REFERENCE');print('\n'.join(f'{v.address-b:x}: {v.mnemonic} {v.op_str}'for v in ins[max(0,j-10):j+20]))
