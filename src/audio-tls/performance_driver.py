from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect,json,re,sys,uuid
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
OUT=ROOT/'analysis/audio-tls/performance-device'
guid=uuid.UUID('88671610-712e-4f1e-84ab-01b5948bd373').bytes_le;cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
out=[]
for name in ['d3d11.dll','d3d11i.dll','d3d11_x.dll','umd.dll']:
    path=ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'/name;p=pefile.PE(str(path));d=p.__data__
    funcs=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in getattr(p,'DIRECTORY_ENTRY_EXCEPTION',[]));starts=[x[0] for x in funcs]
    for half in [guid[:8],guid[8:]]:
        for m in re.finditer(re.escape(half),d):
            rva=p.get_rva_from_offset(m.start());i=bisect.bisect_right(starts,rva)-1;row={'module':name,'half':half.hex(),'rva':hex(rva)}
            if i>=0 and rva<funcs[i][1]:
                a,b=funcs[i];row.update({'procedure':hex(a),'assembly':[f'{x.address-p.OPTIONAL_HEADER.ImageBase:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(p.get_data(a,b-a),p.OPTIONAL_HEADER.ImageBase+a)]})
            out.append(row)
(OUT/'driver-iid-xrefs.json').write_text(json.dumps(out,indent=2))
print(json.dumps(out,indent=2))
