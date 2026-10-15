from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
r=project_root(__file__);add_python_libraries(__file__)
import pefile
for n in ['xg_x.dll','xg.dll']:
 pe=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/n))
 print(n)
 for d in pe.DIRECTORY_ENTRY_IMPORT:print(d.dll.decode(),[i.name.decode() if i.name else i.ordinal for i in d.imports])
