from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import project_root
import re,json,datetime
r=project_root(__file__);code=(r/'analysis/graphics-port/304-graphics-vtable-layout.response.c').read_text().split('case 1:')[1].split('case 2:')[0];implemented={int(i,0):fn.strip()for i,fn in re.findall(r'param_1\[(0x[0-9a-f]+|\d+)\]\s*=\s*([^;]+);',code)};implemented.update({0:'FUN_1400db760',1:'FUN_1400da960',2:'FUN_1400da9e0'});rows=json.load(open(r/'analysis/graphics-port/context1-sdk-methods.json'))
for row in rows:
 old=row['old_slot'];std=row['standard_slot'];target=(old if old<0x19 else old-1);row['az_slot']=None if old==0x19 else target;row['az_backend']=implemented.get(target,'default explicit unsupported method')if old!=0x19 else'FM5ApplyNativeTopology public-inline-ABI policy';row['adapter']='typed forwarding'
 if old==0x14:row['adapter']='reorder buffer,format,offset into format,buffer,offset'
 if row['name']=='DrawIndexed':row['adapter']='pack count/start into uint64; preserve signed base vertex'
 if row['name']=='DrawIndexedInstanced':row['adapter']='pack count/start and firstInstance/baseVertex; move instanceCount to R9'
 if row['name']=='DrawInstanced':row['adapter']='pack firstInstance/startVertex; move instanceCount to R9'
 if old==0x19:row['adapter']='root topology callback, explicit failure outside supported topologies'
 if old==3:row['adapter']='wrap owned Device result through shared Device cache'
 if old<3:row['adapter']='shared proxy identity/refcount'
(r/'analysis/graphics-port/context-method-map.json').write_text(json.dumps({'scope':'Legacy standard Context1 methods mapped to Az 0.9.9; API forwarding harness validates ABI, not rendering; native unsupported methods remain unsupported','methods':rows},indent=2))
print('Mapped methods',len(rows),'Az implemented targets',sum('unsupported'not in x['az_backend']for x in rows))
