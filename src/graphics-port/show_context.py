from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
p=project_root(__file__) / 'analysis/graphics-port/304-graphics-vtable-layout.response.c'
s=p.read_text();i=s.index('case 1:');print(s[i:i+6500])
