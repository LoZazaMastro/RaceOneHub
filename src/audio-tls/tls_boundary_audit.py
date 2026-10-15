from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect,hashlib,json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
OUT=ROOT/'analysis/audio-tls/tls-boundary';OUT.mkdir(exist_ok=True)
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
files=[('host',ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe'),('game',ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe')]
summary={}
for label,path in files:
    pe=pefile.PE(str(path));base=pe.OPTIONAL_HEADER.ImageBase
    functions=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in pe.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0] for x in functions]
    tls=pe.DIRECTORY_ENTRY_TLS.struct
    template=pe.get_data(tls.StartAddressOfRawData-base,tls.EndAddressOfRawData-tls.StartAddressOfRawData)
    rows=[];raw=path.read_bytes()
    patterns=[bytes([0x65,rex,0x8b,(reg<<3)|4,0x25,0x58,0,0,0]) for rex in [0x48,0x4c] for reg in range(8)]
    offsets=set()
    for pattern in patterns:
        at=raw.find(pattern)
        while at>=0:offsets.add(at);at=raw.find(pattern,at+1)
    for at in sorted(offsets):
        rva=pe.get_rva_from_offset(at);i=bisect.bisect_right(starts,rva)-1
        a,b=functions[i] if i>=0 and rva<functions[i][1] else (rva,rva+180)
        rows.append({'rva':hex(rva),'function':hex(a),'assembly':[f'{x.address-base:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(pe.get_data(max(a,rva-40),min(b,rva+200)-max(a,rva-40)),base+max(a,rva-40))]})
    summary[label]={'path':str(path),'sha256':hashlib.sha256(raw).hexdigest(),'tls_index_rva':hex(tls.AddressOfIndex-base),'template_bytes':len(template),'template_hex':template.hex(),'gs58_sequences':len(rows)}
    (OUT/f'{label}-accesses.json').write_text(json.dumps(rows,indent=2))
(OUT/'summary.json').write_text(json.dumps(summary,indent=2));print(json.dumps(summary,indent=2))
