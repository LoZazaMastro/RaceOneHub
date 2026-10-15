from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
r=project_root(__file__);js=json.loads((r/'analysis/graphics-port/memory-counter-loop-candidates.json').read_text());keep=[]
for x in js:
 a=x['assembly']
 if len(a)>12:continue
 if any(': '+k in z for z in a[:-1] for k in ['ret','j','call','inc','dec','add ','mov dword ptr','mov qword ptr','shl','shr','sar','cqo','lea ']):continue
 keep.append(x)
print(json.dumps(keep,indent=2));(r/'analysis/graphics-port/pure-poll-candidates.json').write_text(json.dumps(keep,indent=2))
