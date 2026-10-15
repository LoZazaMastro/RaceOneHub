from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
import sys,uuid
r=project_root(__file__);add_python_libraries(__file__);import pefile
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True)
for a in [0x7640e8,0x764118,0x7640d8,0x7640f8,0x764128]:print(hex(a),uuid.UUID(bytes_le=p.get_data(a,16)))
