from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/fh2_private_prefix.py').read_text().split('for f,n')[0]);import json
j=json.load(open('analysis/graphics-port/native-context-tables-comparison.json'));t=next(x for x in j[1]['tables']if x['rva']=='0xd6950');f=int(t['entries'][0x97],16);print(hex(f));print('\n'.join(f'{i.address-0x180000000:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(f,130),0x180000000+f)))
