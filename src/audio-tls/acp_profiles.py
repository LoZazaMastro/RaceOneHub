from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_game
import struct
import json
ROOT=Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile
report=[]
for name,path in [('FM5',ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'),
                  ('FH2',external_fh2_game() / 'Mount/forza_durango_release_final.exe')]:
    raw=path.read_bytes();pe=pefile.PE(data=raw);found={}
    patterns=[b'AcpHalAllocateShapeContexts',b'AcpHalCreate',b'AcpHal.dll',b'acphal.dll',b'ApuAlloc',struct.pack('<6I',512,0,0,1024,512,0)]
    for pat in patterns:
        pos=0;hits=[]
        while (pos:=raw.find(pat,pos))>=0:
            hits.append({'offset':hex(pos),'rva':hex(pe.get_rva_from_offset(pos))});pos+=1
        found[pat.hex() if b'\0' in pat else pat.decode()]=hits
    report.append({'name':name,'patterns':found})
(ROOT/'analysis/audio-tls/acp-game-profiles.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
