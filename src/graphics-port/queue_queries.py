from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);s=r/'analysis/rea/session'
for n,a in [('310-graphics-query-begin','0x1401b14c0'),('311-graphics-query-end','0x1401b17a0'),('312-graphics-query-getdata','0x1401b1940'),('313-graphics-context-flush','0x1401b1110')]:
 (s/(n+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':a}}))
