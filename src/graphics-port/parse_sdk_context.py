from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import re,json
r=project_root(__file__);s=(r/'analysis/graphics-port/sdk_types.i').read_text();s=s.split('typedef struct ID3D11DeviceContext1Vtbl',1)[1].split('} ID3D11DeviceContext1Vtbl',1)[0];s=re.sub(r'__declspec\([^\n]*?\)\s*','',s);s=re.sub(r'\s+',' ',s)
ms=re.findall(r'([A-Za-z_][A-Za-z_0-9\s*]*?)\(\s*__stdcall\s*\*(\w+)\s*\)\s*\((.*?)\);',s)
out=[]
for j,(ret,name,params)in enumerate(ms):
 ps=[p.strip()for p in params.split(',')];row={'standard_slot':j,'old_slot':j+1 if j>=7 else j,'az_slot':j+1 if 7<=j<24 else j,'return':ret.strip().replace('BEGIN_INTERFACE','').strip(),'name':name,'params':ps};out.append(row)
(r/'analysis/graphics-port/context1-sdk-methods.json').write_text(json.dumps(out,indent=2));print(len(out));print(out[:10]);print(out[-3:])
