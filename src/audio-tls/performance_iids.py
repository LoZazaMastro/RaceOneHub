from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import sys,json,uuid,re
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile
p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/d3d11_x.dll'))
out=[]
for s in p.DIRECTORY_ENTRY_EXPORT.symbols:
    if s.name and s.name.startswith(b'IID_'):
        out.append({'name':s.name.decode(),'guid':str(uuid.UUID(bytes_le=p.get_data(s.address,16)))})
(ROOT/'analysis/audio-tls/performance-device/original-iids.json').write_text(json.dumps(out,indent=2))
print(json.dumps([x for x in out if any(n in x['name'] for n in ['Scratch','Memory','Allocator','Pool','Counter','Performance'])],indent=2))
