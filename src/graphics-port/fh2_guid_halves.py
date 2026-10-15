from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import external_fh2_root, project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0])
import uuid,re
p=pefile.PE(str(external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True)
for g in ['9458fe06-c78d-47f7-96a0-ec7b727be1e9','48800095-7134-4be7-9186-b86becb26477','c0bfa96c-e089-44fb-8eaf-26f8796190da']:
 b=uuid.UUID(g).bytes_le
 print(g,[(j,[hex(p.get_rva_from_offset(m.start()))for m in re.finditer(re.escape(b[j:j+8]),p.__data__)])for j in [0,8]])
