from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_game
import json,hashlib,sys
ROOT=Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile
paths=[('FM5',ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/Microsoft.Xbox.Services.dll'),
('FH2',external_fh2_game() / 'Mount/Microsoft.Xbox.Services.dll')]
report=[]
for name,path in paths:
    data=path.read_bytes();p=pefile.PE(data=data)
    row={'name':name,'path':str(path),'sha256':hashlib.sha256(data).hexdigest(),'bytes':len(data),
    'machine':hex(p.FILE_HEADER.Machine),'timestamp':hex(p.FILE_HEADER.TimeDateStamp),'sections_count':p.FILE_HEADER.NumberOfSections,
    'image_size':hex(p.OPTIONAL_HEADER.SizeOfImage),'imagebase':hex(p.OPTIONAL_HEADER.ImageBase),'entry':hex(p.OPTIONAL_HEADER.AddressOfEntryPoint),
    'directories':[{'index':i,'name':x.name,'rva':hex(x.VirtualAddress),'bytes':hex(x.Size)} for i,x in enumerate(p.OPTIONAL_HEADER.DATA_DIRECTORY)],
    'tls':p.DIRECTORY_ENTRY_TLS.struct.dump() if hasattr(p,'DIRECTORY_ENTRY_TLS') else None,
    'imports':[{'dll':d.dll.decode(),'symbols':[s.name.decode() if s.name else '#'+str(s.ordinal) for s in d.imports]} for d in getattr(p,'DIRECTORY_ENTRY_IMPORT',[])],
    'delay_imports':[{'dll':d.dll.decode(),'symbols':[s.name.decode() if s.name else '#'+str(s.ordinal) for s in d.imports]} for d in getattr(p,'DIRECTORY_ENTRY_DELAY_IMPORT',[])],
    'exports':[{'name':s.name.decode() if s.name else None,'rva':hex(s.address)} for s in getattr(getattr(p,'DIRECTORY_ENTRY_EXPORT',None),'symbols',[])]}
    report.append(row)
(ROOT/'analysis/audio-tls/xsapi-pe-comparison.json').write_text(json.dumps(report,indent=2))
for row in report:
    print(json.dumps({k:v for k,v in row.items() if k not in ['imports','delay_imports','exports']},indent=2))
