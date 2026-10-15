from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, project_root
r=project_root(__file__);add_python_libraries(__file__)
import pefile
p=pefile.PE(str(r/'runtime/azimuth-0.9.9-original/azimuth.exe'));print(p.get_data(0x58090,32).hex());print(hex(p.OPTIONAL_HEADER.SizeOfImage))
