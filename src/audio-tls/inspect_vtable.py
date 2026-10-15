from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json
import struct
ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile
pe = pefile.PE(str(ROOT / 'runtime/azimuth-0.9.9-original/azimuth.exe'))
v = struct.unpack('<16Q', pe.get_data(0x2b5608,128))
report = [{'slot':i, 'offset':hex(i*8), 'va':hex(p), 'rva':hex(p-pe.OPTIONAL_HEADER.ImageBase)} for i,p in enumerate(v)]
(ROOT / 'analysis/audio-tls/engine-vtable.json').write_text(json.dumps(report,indent=2))
print(json.dumps(report,indent=2))
for n,label,addr in [(406,'mastering-voice',v[7]),(407,'source-voice',v[5]),(408,'engine-ref',v[2])]:
    p=ROOT/'analysis/rea/session'/f'{n}-audio-tls-{label}.request.json'
    if not p.exists():
        t=p.with_suffix('.pending');t.write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':hex(addr)}}));t.replace(p)
