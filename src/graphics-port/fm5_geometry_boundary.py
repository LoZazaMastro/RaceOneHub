from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/fm5_draw_boundary.py').read_text().split('for f,n')[0])
for f,n in [(0x7aec,190),(0x7b90,18),(0x7b98,35),(0x7d88,40),(0x7e60,40)]:print('\nFUNC',hex(f));print('\n'.join(f'{i.address-0x180000000:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(f,n),0x180000000+f)))
