from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
p=project_root(__file__) / 'analysis/graphics-port/umd-debug-abi-strings.json';a=json.loads(p.read_text())
for x in a:
 if any(s in x['text'] for s in ['PartialFlush','PerformanceContext','PrivateDataInterface','SetAuto','AutoFlush']):print(x)
