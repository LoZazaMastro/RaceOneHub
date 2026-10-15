from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile,capstone
from capstone.x86 import X86_OP_MEM
p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'));base=p.OPTIONAL_HEADER.ImageBase;cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True;cs.skipdata=True
sec=p.sections[0];ins=list(cs.disasm(sec.get_data(),base+sec.VirtualAddress));out=[]
for i,x in enumerate(ins):
    if not x.id:continue
    if any(o.type==X86_OP_MEM and o.mem.disp==0x49c08 for o in x.operands):out.append({'rva':hex(x.address-base),'assembly':[f'{q.address-base:x}: {q.mnemonic} {q.op_str}' for q in ins[max(0,i-8):i+8]]})
(ROOT/'analysis/audio-tls/performance-device/normal-device-construction.json').write_text(json.dumps(out,indent=2));print(json.dumps(out,indent=2))
