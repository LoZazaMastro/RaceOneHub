from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,re,json,struct,uuid,bisect
r=project_root(__file__);add_python_libraries(__file__)
import pefile,capstone
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
for name in ['d3d11.dll','d3d11i.dll']:
 p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/name;pe=pefile.PE(str(p));raw=pe.__data__;base=pe.OPTIONAL_HEADER.ImageBase
 print(name)
 for guid in ['48800095-7134-4be7-9186-b86becb26477','177700f9-876a-4436-b368-36a604f82cef']:
  b=uuid.UUID(guid).bytes_le
  for j in [0,8]:
   k=0;sites=[]
   while (k:=raw.find(b[j:j+8],k))>=0:
    sites.append(hex(pe.get_rva_from_offset(k)));k+=1
   print(guid,'half',j,sites)
