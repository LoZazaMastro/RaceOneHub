from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import sys,re,json
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile
out=[]
for path in (ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32').glob('*.dll'):
    if not any(x in path.name.lower() for x in ['d3d','umd']):continue
    p=pefile.PE(str(path))
    for m in re.finditer(rb'[\x20-\x7e]{6,}',p.__data__):
        if b'Scratch' in m.group() or b'scratch' in m.group():out.append({'module':path.name,'rva':hex(p.get_rva_from_offset(m.start())),'text':m.group().decode()})
(ROOT/'analysis/audio-tls/performance-device/scratch-native-strings.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
