from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import re,sys,json
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile
p=pefile.PE(str(ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe'));out=[]
for m in re.finditer(rb'[\x20-\x7e]{7,}',p.__data__):
    text=m.group().decode()
    if any(x in text for x in ['Scratch','scratch','CreateMemory','MemoryPool','Allocator','Pool','CreateCommand','PerformanceDevice','ESRAM','MemoryAlloc','Allocation']):out.append({'rva':hex(p.get_rva_from_offset(m.start())),'text':text})
(ROOT/'analysis/audio-tls/performance-device/az-memory-strings.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
