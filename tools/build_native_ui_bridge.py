"""Relink the exact frozen FM5 bridge, replacing only its native UI binding.
Never rebuilds the host or any graphics object, and never launches the game.
"""
from pathlib import Path
import hashlib,json,subprocess
ROOT=Path(__file__).resolve().parents[1]
BUILD=ROOT/'analysis/pc-build';STAGE=ROOT/'analysis/ui-port/msaa-visible-124'
sha=lambda p:hashlib.sha256(Path(p).read_bytes()).hexdigest()
def main():
 recipe=json.loads((BUILD/'build.json').read_text());live=ROOT/'runtime/pc/ForzaMastro5Bridge.dll'
 if sha(live)!=recipe['sha256']:raise RuntimeError('Current runtime no longer matches frozen bridge recipe')
 STAGE.mkdir(parents=True,exist_ok=True)
 inputs=[BUILD/(Path(s).stem+'.obj') for s in recipe['sources']]
 inputs += [Path(s) for s in recipe['command'] if s.lower().endswith('.obj')]
 if len(inputs)!=len(set(inputs)) or any(not p.is_file() for p in inputs):raise RuntimeError('Frozen object set incomplete/ambiguous')
 frozen={str(p):sha(p) for p in inputs}
 def relink(directory,replacement=None):
  directory.mkdir(exist_ok=True);args=[]
  for value in recipe['command']:
   if value in [str(ROOT/s) for s in recipe['sources']]:
    item=BUILD/(Path(value).stem+'.obj')
    if replacement and item.stem=='fm5_native_binding':item=replacement
    args.append(str(item))
   elif value.startswith('/Fe'):args.append('/Fe'+str(directory/'ForzaMastro5Bridge.dll'))
   elif value.startswith('/Fo'):args.append('/Fo'+str(directory)+'\\')
   elif value.startswith('/MAP:'):args.append('/MAP:'+str(directory/'bridge.map'))
   elif value.startswith('/IMPLIB:'):args.append('/IMPLIB:'+str(directory/'ForzaMastro5Bridge.lib'))
   else:args.append(value)
  result=subprocess.run(args,cwd=directory,capture_output=True,text=True)
  (directory/'link.log').write_text(result.stdout+result.stderr)
  if result.returncode:raise RuntimeError(result.stdout+result.stderr)
  return directory/'ForzaMastro5Bridge.dll'
 baseline=relink(STAGE/'baseline')
 if sha(baseline)!=sha(live):raise RuntimeError('Frozen-object baseline does not reproduce the actual current DLL byte for byte')
 command=recipe['command'];first=next(i for i,s in enumerate(command) if s.lower().endswith(('.c','.cpp')))
 prefix=[x for x in command[:first] if x!='/LD']
 obj=STAGE/'fm5_native_binding.obj'
 compile=prefix+['/c',str(ROOT/'src/native-ui/fm5_native_binding.c'),'/Fo'+str(obj)]
 result=subprocess.run(compile,cwd=STAGE,capture_output=True,text=True)
 (STAGE/'binding-compile.log').write_text(result.stdout+result.stderr)
 if result.returncode:raise RuntimeError(result.stdout+result.stderr)
 candidate=relink(STAGE/'candidate',obj)
 for path,digest in frozen.items():
  if sha(path)!=digest:raise RuntimeError('Frozen object was changed')
 from project_paths import add_python_libraries
 add_python_libraries(__file__)
 import pefile
 old=pefile.PE(str(live));new=pefile.PE(str(candidate))
 exports=lambda p:[(e.name.decode() if e.name else None,e.ordinal) for e in p.DIRECTORY_ENTRY_EXPORT.symbols]
 if exports(old)!=exports(new):raise RuntimeError('Export name/ordinal ABI changed')
 if new.OPTIONAL_HEADER.DllCharacteristics!=old.OPTIONAL_HEADER.DllCharacteristics:raise RuntimeError('DLL security flags changed')
 report={'baseline_reproduced_actual_DLL_byte_for_byte':True,'baseline_sha256':sha(live),'candidate':str(candidate),'candidate_sha256':sha(candidate),'candidate_bytes':candidate.stat().st_size,'exports_name_and_ordinals_unchanged':True,'export_count':len(exports(new)),'dll_security_flags_unchanged':True,'recompiled_sources':['src/native-ui/fm5_native_binding.c'],'binding_source_sha256':sha(ROOT/'src/native-ui/fm5_native_binding.c'),'unchanged_input_object_hashes':frozen,'game_launched':False,'host_rebuilt':False,'published':False}
 (STAGE/'relink-report.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({k:v for k,v in report.items() if k!='unchanged_input_object_hashes'},indent=2))
if __name__=='__main__':main()
