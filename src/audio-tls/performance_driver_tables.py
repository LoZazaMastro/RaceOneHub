from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,re,struct,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile
out=[]
for name,target in [('d3d11.dll',0x118a20),('d3d11i.dll',0x15f9d0)]:
    p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/name));raw=p.__data__;base=p.OPTIONAL_HEADER.ImageBase
    for m in re.finditer(re.escape(struct.pack('<Q',base+target)),raw):
        start=m.start()-64;rva=p.get_rva_from_offset(start)
        out.append({'module':name,'ref':hex(p.get_rva_from_offset(m.start())), 'rows':[{'rva':hex(rva+i*8),'qword':hex(struct.unpack_from('<Q',raw,start+i*8)[0])} for i in range(24)]})
(ROOT/'analysis/audio-tls/performance-device/driver-guid-pointer-tables.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
