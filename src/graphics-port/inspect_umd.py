from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,uuid,re,json
r=project_root(__file__);add_python_libraries(__file__);import pefile
p=r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll';pe=pefile.PE(str(p),fast_load=True);raw=pe.__data__;g=uuid.UUID('9458fe06-c78d-47f7-96a0-ec7b727be1e9').bytes_le
print('Sections',[(s.Name.decode().strip('\x00'),hex(s.VirtualAddress),s.Misc_VirtualSize)for s in pe.sections]);print('GUID',[(hex(pe.get_rva_from_offset(m.start()))) for m in re.finditer(re.escape(g),raw)])
for m in re.finditer(rb'[\x20-\x7e]{7,}',raw):
 s=m.group().decode()
 if 'PerformanceContext' in s or ('Context' in s and '.?AV' in s):print(hex(pe.get_rva_from_offset(m.start())),s)
