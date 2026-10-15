from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,uuid
r=project_root(__file__);add_python_libraries(__file__);import pefile
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/d3d11_x.dll'))
for e in p.DIRECTORY_ENTRY_EXPORT.symbols:
 if e.name and e.name.startswith(b'IID_'):
  g=str(uuid.UUID(bytes_le=p.get_data(e.address,16)))
  if g=='9458fe06-c78d-47f7-96a0-ec7b727be1e9':print(e.name.decode(),g)
