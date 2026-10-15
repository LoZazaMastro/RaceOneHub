from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
ROOT=Path(__file__).resolve().parents[3]
add_python_libraries(__file__)
import pefile
p=pefile.PE(str(ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe'))
functions=[0xe8700,0xea870,0xea000,0xea570,0xeab70,0xeac00,0xeab90,0xeabd0,0x121c40,0x122090]
rows=['static const struct { DWORD rva; BYTE bytes[16]; } resource_guards[10] = {']
for rva in functions: rows.append('    {0x%x,{%s}},'%(rva,','.join('0x%02x'%x for x in p.get_data(rva,16))))
rows.append('};')
(Path(__file__).parent/'resource_guards.h').write_text('\n'.join(rows)+'\n')
