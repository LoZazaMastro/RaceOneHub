from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
add_python_libraries(__file__)
import pefile
p=pefile.PE(str(ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe'))
rvas=[0x182a0,0x13ea0,0x6f00,0x6eb0,0x7bd0,0x7f6f0,0x31e0,0x74660,0x8d50,0x6d1d0,0x6d560,0x1538f0,0x875b0,0x7140,0x7070,0x49b80,0x57f80]
s='#ifndef FM5_UI_GUARDS_H\n#define FM5_UI_GUARDS_H\nstatic const struct { unsigned rva; unsigned char bytes[16]; } fm5_ui_guards[]={\n'
for r in rvas:s+='{0x%x,{%s}},\n'%(r,','.join('0x%02x'%x for x in p.get_data(r,16)))
s+='};\n#define FM5_UI_GUARD_COUNT (sizeof(fm5_ui_guards)/sizeof(fm5_ui_guards[0]))\n#endif\n'
(HERE/'fm5_ui_guards.h').write_text(s)
