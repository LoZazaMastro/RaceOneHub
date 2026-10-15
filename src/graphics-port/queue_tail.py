from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
s=project_root(__file__)/'analysis/rea/session'
for n,a in [('336-graphics-wait-resource','0x14012cf90'),('337-graphics-user-constants','0x1401b5b50'),('338-graphics-extra-resource','0x1401b3980')]:
 (s/(n+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':a}}))
