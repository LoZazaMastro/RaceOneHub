from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);s=r/'analysis/rea/session'
for n,a in [('307-graphics-host-wait-inner','0x1400c31b0'),('308-graphics-host-wait-xrefs','0x1400c31b0'),('309-graphics-host-device-owner','0x140055870')]:
 name='xrefs' if 'xrefs' in n else 'procedure_pseudo_code';args={'address':a} if name=='xrefs' else {'procedure':a}
 (s/(n+'.request.json')).write_text(json.dumps({'name':name,'arguments':args}))
