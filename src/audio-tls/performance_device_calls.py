from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect,json,struct,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
OUT=ROOT/'analysis/audio-tls/performance-device'
path=ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe';p=pefile.PE(str(path))
fs=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in p.DIRECTORY_ENTRY_EXCEPTION);st=[x[0] for x in fs]
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);out=[]
for s in p.sections:
    if not s.Characteristics&0x20000000:continue
    d=s.get_data();pos=0
    while (pos:=d.find(b'\xe8',pos))>=0:
        a=s.VirtualAddress+pos
        if pos+5<=len(d) and a+5+struct.unpack_from('<i',d,pos+1)[0]==0x6bb200:
            i=bisect.bisect_right(st,a)-1
            if i>=0 and a<fs[i][1]:
                b,e=fs[i];out.append({'call_rva':hex(a),'procedure_rva':hex(b),'assembly':[f'{x.address-p.OPTIONAL_HEADER.ImageBase:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(p.get_data(b,e-b),p.OPTIONAL_HEADER.ImageBase+b)]})
        pos+=1
(OUT/'qi-callers.json').write_text(json.dumps(out,indent=2))
for row in out:
    print('CALL',row['call_rva'],'function',row['procedure_rva'])
    for i,line in enumerate(row['assembly']):
        if row['call_rva'][2:]+':' in line:print('\n'.join(row['assembly'][max(0,i-10):i+8]))
