from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/fh2_private_prefix.py').read_text().split('for f,n')[0])
for f,n in [(0x9590,65),(0x9c80,75),(0x9ec0,75),(0xa040,65),(0xa080,65)]:print('\nFUNC',hex(f));print('\n'.join(f'{i.address-0x180000000:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(f,n),0x180000000+f)))
