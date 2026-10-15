from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import external_fh2_root, project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0]);import json,struct
j=json.load(open('analysis/graphics-port/native-context-tables-comparison.json'))
for label,path,t in [('FM5',r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',j[0]['tables'][0]),('FH2',external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',next(x for x in j[1]['tables']if x['rva']=='0xd6950'))]:
 p=pefile.PE(str(path),fast_load=True)
 for idx in ([0x92,0x9b,0x9c,0x9d,0xa1,0xa4,0xad]if label=='FM5'else[0xa6,0xa7,0xa8,0xa9,0xaa,0xab,0xac,0xad,0xae,0xaf,0xb0,0xb1,0xb2]):
  f=int(t['entries'][idx],16);print('\n',label,hex(idx),hex(f));print('\n'.join(f'{i.address-0x180000000:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(f,95),0x180000000+f)))
