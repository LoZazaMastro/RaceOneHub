from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/fm5_draw_boundary.py').read_text().split('for f,n')[0]);import json
j=json.load(open('analysis/graphics-port/native-context-tables-comparison.json'));t=j[0]['tables'][0]
for idx in [0x93,0x94,0x95,0x96,0x97,0x98,0xc0,0xc1,0xc2,0xc3,0xc4]:
 f=int(t['entries'][idx],16);ins=list(cs.disasm(p.get_data(f,16),0x180000000+f))
 if ins and ins[0].mnemonic=='jmp':f=int(ins[0].op_str,16)-0x180000000
 print('\nSLOT',hex(idx),'FUNCTION',hex(f));print('\n'.join(f'{i.address-0x180000000:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(f,250),0x180000000+f)))
