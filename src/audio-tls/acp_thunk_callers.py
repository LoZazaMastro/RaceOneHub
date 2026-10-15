from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,struct,sys,bisect
ROOT=Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile
from capstone import Cs,CS_ARCH_X86,CS_MODE_64
p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'))
cs=Cs(CS_ARCH_X86,CS_MODE_64)
target=0xe5b4f8
functions=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in p.DIRECTORY_ENTRY_EXCEPTION)
starts=[x[0] for x in functions];out=[]
for s in p.sections:
    if not s.Characteristics&0x20000000:continue
    d=s.get_data();pos=0
    while (pos:=d.find(b'\xe8',pos))>=0:
        rva=s.VirtualAddress+pos
        if pos+5<=len(d) and rva+5+struct.unpack_from('<i',d,pos+1)[0]==target:
            i=bisect.bisect_right(starts,rva)-1
            if i>=0 and functions[i][0]<=rva<functions[i][1]:
                a,b=functions[i];assembly=[f'{x.address-p.OPTIONAL_HEADER.ImageBase:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(p.get_data(a,b-a),p.OPTIONAL_HEADER.ImageBase+a)]
                out.append({'call_rva':hex(rva),'procedure_rva':hex(a),'assembly':assembly})
        pos+=1
(ROOT/'analysis/audio-tls/acp-thunk-callers.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
