from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,json,hashlib
r=project_root(__file__);add_python_libraries(__file__)
import pefile,capstone
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'));c=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);c.detail=True
out=[]
for rva,new in [(0x9f334,0x1b000),(0x9f341,0xc738)]:
 i=next(c.disasm(p.get_data(rva,10),0x140000000+rva));before=i.bytes;after=bytearray(before);off=i.encoding.imm_offset;after[off:off+4]=new.to_bytes(4,'little')
 out.append({'instruction_rva':hex(rva),'guard':before.hex(),'replacement':after.hex(),'immediate_rva':hex(rva+off),'purpose':'FM5 original XG PE image size' if rva==0x9f334 else 'FM5 original XG PE entrypoint'})
(r/'analysis/graphics-port/xg-layout-patch-recipe.json').write_text(json.dumps({'host_sha256':hashlib.sha256(p.__data__).hexdigest(),'preserves':'All layout guards, import binding, relocation and original process attach remain active. Only exact expected size/entry constants change.','patches':out},indent=2));print(json.dumps(out,indent=2))
