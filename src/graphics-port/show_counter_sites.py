from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);js=json.loads((r/'analysis/graphics-port/fm5-context-call-candidates.json').read_text())
for g in js[:3]:
 print('SLOT',g['slot'])
 for x in g['sites']:
  if g['slot']=='0xd6' or x['rva'] in ['0x8e1baa','0x8e1be0','0x8e1c61','0x9214a4','0x921480']:
   print('\n'.join(x['assembly']))
