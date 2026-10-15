from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import json
s=project_root(__file__)/'analysis/rea/session';(s/'347-graphics-dxgi-legacy-create.request.json').write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':'0x14004f7c0'}}));(s/'348-graphics-dxgi-core-window.request.json').write_text(json.dumps({'name':'procedure_pseudo_code','arguments':{'procedure':'0x14004f730'}}))
