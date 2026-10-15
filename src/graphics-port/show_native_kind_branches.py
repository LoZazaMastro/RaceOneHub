from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0]);import struct
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'),fast_load=True)
print([(i,hex(struct.unpack('<I',p.get_data(0xf4144+i*4,4))[0]))for i in range(18)])
