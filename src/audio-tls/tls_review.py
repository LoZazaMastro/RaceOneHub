from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import sys,bisect,json
r=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
p=pefile.PE(str(r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'));base=p.OPTIONAL_HEADER.ImageBase;c=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);c.detail=True
fs=[(x.struct.BeginAddress,x.struct.EndAddress) for x in p.DIRECTORY_ENTRY_EXCEPTION];starts=[a for a,b in fs]
for q in [0xa1c104,0xa3bab3,0x7780ed,0x7e1d20]:
 a,b=fs[bisect.bisect_right(starts,q)-1];o='\n'.join(f'{i.address-base:x}: {i.mnemonic} {i.op_str}' for i in c.disasm(p.get_data(a,b-a),base+a));(r/f'analysis/audio-tls/tls-boundary/review-{q:x}.txt').write_text(o);print(hex(q),hex(a),hex(b),o[:22000],sep='\n')
