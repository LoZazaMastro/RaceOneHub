from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re,struct,bisect,json
r=project_root(__file__);add_python_libraries(__file__);import pefile,capstone
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'),fast_load=True);p.parse_data_directories(directories=[3]);rs=sorted((x.struct.BeginAddress,x.struct.EndAddress)for x in p.DIRECTORY_ENTRY_EXCEPTION);st=[a for a,z in rs];s=next(s for s in p.sections if s.Name.rstrip(b'\0')==b'.text');raw=s.get_data();b=p.OPTIONAL_HEADER.ImageBase;cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);rows=[]
for idx in range(0x87,0xc8):
 disp=0x20+8*idx;sites=[]
 for m in re.finditer(rb'\xff[\x90-\x97]'+re.escape(struct.pack('<I',disp)),raw):
  at=s.VirtualAddress+m.start()
  if not 0x690000<=at<0x740000:continue
  ri=bisect.bisect_right(st,at)-1;a,z=rs[ri];ins=list(cs.disasm(p.get_data(a,z-a),b+a));k=next((i for i,x in enumerate(ins)if x.address==b+at),None)
  if k is None:continue
  asm=[f'{x.address-b:x}: {x.mnemonic} {x.op_str}'for x in ins[max(0,k-12):min(len(ins),k+4)]];sites.append({'site':hex(at),'function':hex(a),'asm':asm})
 if sites:rows.append({'old_slot':hex(idx),'offset':hex(disp),'sites':sites});print(hex(idx),len(sites),[x['site']for x in sites])
(r/'analysis/graphics-port/fm5-xbox-context-tail-candidates.json').write_text(json.dumps(rows,indent=2))
