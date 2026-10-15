from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import external_fh2_root, project_root
import re
r=project_root(__file__)
for label,path in [('FM5',r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd_d.dll'),('FH2',external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd_d.dll')]:
 if not path.exists():continue
 raw=path.read_bytes();print(label)
 for regex in [rb'[\x20-\x7e]{6,}',rb'(?:[\x20-\x7e]\x00){6,}']:
  for m in re.finditer(regex,raw):
   s=m.group().decode('utf-16le' if b'\x00' in m.group() else 'ascii')
   if any(x.lower()in s.lower()for x in ['topology','DrawIndexed','SetVertexBuffers','SetShaderResources','ConstantBuffers']):print(hex(m.start()),s[:300])
