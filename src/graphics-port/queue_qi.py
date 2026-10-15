from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);s=r/'analysis/rea/session'
for n,a in [('305-graphics-query-interface','0x1400db760')]:
 (s/(n+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':a}}))
