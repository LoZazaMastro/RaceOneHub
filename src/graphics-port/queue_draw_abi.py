from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);s=r/'analysis/rea/session'
for n,a in [('326-graphics-index-buffer','0x1401b3840'),('327-graphics-draw-indexed-instanced','0x14012c120'),('328-graphics-draw-instanced','0x14012bf90'),('329-graphics-draw-indexed','0x14012be10'),('330-graphics-draw','0x14012bca0')]:
 (s/(n+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':a}}))
