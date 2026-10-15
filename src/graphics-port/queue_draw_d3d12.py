from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
s=project_root(__file__)/'analysis/rea/session'
for n,a in [('334-graphics-d3d12-draw-indexed','0x1400edf00'),('335-graphics-d3d12-draw','0x1400ede90')]:
 (s/(n+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':a}}))
