from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
s=project_root(__file__)/'analysis/rea/session'
for n,a in [('331-graphics-draw-indexed-core','0x14012bae0'),('332-graphics-draw-core','0x14012b970'),('333-graphics-draw-instanced-core','0x14012ba30')]:
 (s/(n+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':a}}))
