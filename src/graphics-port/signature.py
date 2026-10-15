from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
r=project_root(__file__);add_python_libraries(__file__)
import pefile
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'));b=p.get_data(0x58090,256)
(r/'src/graphics-port/factory_signature.h').write_text('static const unsigned char factory_signature[] = {\n'+',\n'.join('    '+','.join('0x%02x'%x for x in b[i:i+16]) for i in range(0,len(b),16))+'\n};\n')
