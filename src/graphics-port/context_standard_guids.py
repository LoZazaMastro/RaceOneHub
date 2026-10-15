from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re,uuid,bisect,json,struct
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
out=[]
for name in ['umd.dll','umd_d.dll','d3d11.dll']:
 p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/name),fast_load=True);p.parse_data_directories(directories=[3]);b=p.OPTIONAL_HEADER.ImageBase;rs=sorted((x.struct.BeginAddress,x.struct.EndAddress)for x in p.DIRECTORY_ENTRY_EXCEPTION);st=[x[0]for x in rs]
 for guid in ['c0bfa96c-e089-44fb-8eaf-26f8796190da','48800095-c038-4f31-8a73-3a1acfff9c47','9458fe06-c78d-47f7-96a0-ec7b727be1e9']:
  ads={b+p.get_rva_from_offset(m.start()) for m in re.finditer(re.escape(uuid.UUID(guid).bytes_le),p.__data__)};refs=[]
  for s in p.sections:
   if not s.Characteristics&0x20000000:continue
   for i in cs.disasm(s.get_data(),b+s.VirtualAddress):
    if any(o.type==capstone.CS_OP_MEM and o.mem.base==capstone.x86.X86_REG_RIP and i.address+i.size+o.mem.disp in ads for o in i.operands):
     idx=bisect.bisect_right(st,i.address-b)-1;refs.append({'at':hex(i.address-b),'func':hex(rs[idx][0])})
  row={'module':name,'guid':guid,'rvas':[hex(x-b)for x in ads],'refs':refs};out.append(row);print(row)
(r/'analysis/graphics-port/ordinary-context-guids.json').write_text(json.dumps(out,indent=2))
