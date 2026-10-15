from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import sys,json,bisect
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile,capstone
p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'));cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
f=next(x for x in p.DIRECTORY_ENTRY_EXCEPTION if x.struct.BeginAddress<=0x6bb290<x.struct.EndAddress);a,b=f.struct.BeginAddress,f.struct.EndAddress
asm=[f'{x.address-p.OPTIONAL_HEADER.ImageBase:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(p.get_data(a,b-a),p.OPTIONAL_HEADER.ImageBase+a)]
(ROOT/'analysis/audio-tls/performance-device/guest-helper-6bb290.json').write_text(json.dumps(asm,indent=2));print('\n'.join(asm))
