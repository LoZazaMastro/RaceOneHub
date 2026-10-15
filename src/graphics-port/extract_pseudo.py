from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__)
for p in (r/'analysis/rea/session').glob('3*.response.json'):
 j=json.loads(p.read_text()).get('result',{}).get('structuredContent',{}).get('raw_result',{})
 if isinstance(j,dict) and isinstance(j.get('value'),str): (r/'analysis/graphics-port'/p.with_suffix('.c').name).write_text(j['value'])
