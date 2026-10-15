from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re,json,struct,uuid,bisect
r=project_root(__file__);add_python_libraries(__file__)
import pefile,capstone
p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/d3d11.dll';pe=pefile.PE(str(p));raw=pe.__data__;base=pe.OPTIONAL_HEADER.ImageBase;cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
for m in re.finditer(rb'[\x20-\x7e]{8,}',raw):
 if (b'Context' in m.group() or b'Device' in m.group()) and (b'.?AV' in m.group() or b'.?AU' in m.group()):print(hex(pe.get_rva_from_offset(m.start())),m.group().decode())
for g in ['48800095-7134-4be7-9186-b86becb26477','177700f9-876a-4436-b368-36a604f82cef']:
 b=uuid.UUID(g).bytes_le;off=raw.find(b);print(g,'guidrva',hex(pe.get_rva_from_offset(off)) if off>=0 else None)
