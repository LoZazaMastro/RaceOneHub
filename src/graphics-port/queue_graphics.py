from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__)
j=json.loads((r/'analysis/graphics-port/export-assembly.json').read_text())
for p in j:
 print(p['file'])
 print([(x['name'],x['rva']) for x in p['exports'] if x['name'] and (x['name'].startswith('D3D11') or 'Device' in x['name'])])
s=r/'analysis/rea/session'
for label,addr in [('300-graphics-factory','0x140058090')]:
 (s/(label+'.request.json')).write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':addr}}))
