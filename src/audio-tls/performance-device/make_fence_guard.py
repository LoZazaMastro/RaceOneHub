from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
add_python_libraries(__file__)
import pefile
p=pefile.PE(str(ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe'))
(HERE/'fence_guard.h').write_text('static const BYTE native_fence_guard[16]={'+','.join('0x%02x'%x for x in p.get_data(0xc39f0,16))+'};\n')
