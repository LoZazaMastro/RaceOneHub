from pathlib import Path
import hashlib,json,os,subprocess,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
OUT=ROOT/'analysis/audio-tls/resource-port';OUT.mkdir(parents=True,exist_ok=True)
VS=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK=Path(r'C:\Program Files (x86)\Windows Kits\10');version=sorted((SDK/'Include').glob('*'),key=lambda p:tuple(map(int,p.name.split('.'))))[-1].name
cl=VS/'bin/Hostx64/x64/cl.exe';env=os.environ.copy();env['PATH']=str(cl.parent)+os.pathsep+env['PATH']
env['INCLUDE']=';'.join(str(p) for p in [VS/'include',SDK/'Include'/version/'ucrt',SDK/'Include'/version/'shared',SDK/'Include'/version/'um'])
env['LIB']=';'.join(str(p) for p in [VS/'lib/x64',SDK/'Lib'/version/'ucrt/x64',SDK/'Lib'/version/'um/x64'])
subprocess.run([sys.executable,str(HERE/'make_guards.py')],check=True)
common=[str(cl),'/nologo','/W4','/WX','/O2','/MT']
command=common+['/DFM5_RESOURCE_TEST','/DFM5_ALLOCATION_TEST',str(HERE/'test_resource.c'),str(HERE/'fm5_resource.c'),str(ROOT/'src/allocation-port/fm5_allocation.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_resource.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
p=subprocess.run(command,env=env,text=True,capture_output=True);(OUT/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
production=common+['/c',str(HERE/'fm5_resource.c'),'/Fo'+str(OUT/'fm5_resource.production.obj')]
p=subprocess.run(production,env=env,text=True,capture_output=True);(OUT/'production-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
probe=common+[str(HERE/'test_production_guard.c'),str(OUT/'fm5_resource.production.obj'),str(ROOT/'src/allocation-port/fm5_allocation.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_production_guard.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
p=subprocess.run(probe,env=env,text=True,capture_output=True);(OUT/'production-probe-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(OUT/'test_production_guard.exe')],text=True,capture_output=True,timeout=20);(OUT/'production-guard.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
guard_result=json.loads(p.stdout)
p=subprocess.run([str(OUT/'test_resource.exe')],text=True,capture_output=True,timeout=20);(OUT/'native-test.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
assert not p.stderr,p.stderr
report=json.loads(p.stdout);report['compile_command']=command;report['production_command']=production
trace_env=env.copy();trace_env['FM5_TRACE_PLACEMENT']='1'
traced=subprocess.run([str(OUT/'test_resource.exe')],env=trace_env,text=True,capture_output=True,timeout=20);(OUT/'placement-trace.log').write_text(traced.stdout+traced.stderr);assert traced.returncode==0,(traced.returncode,traced.stdout,traced.stderr)
lines=traced.stderr.splitlines();assert len(lines)==16,lines
assert all(line.startswith('FM5_PLACEMENT ') for line in lines),lines
assert any('a=0xe b=0x0' in line and 'desc13=[64,64,7,1,99,1,0,0,8,0,0,0,0]' in line for line in lines),lines
assert json.loads(traced.stdout)==json.loads(p.stdout),(traced.stdout,p.stdout)
report['placement_diagnostics']={'default_silent':True,'environment':'FM5_TRACE_PLACEMENT=1','maximum_records':16,'captured_records':len(lines),'semantics_match_default':True}
report['production_guard']=guard_result
report['sources']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in HERE.glob('*') if p.suffix in ['.c','.h']}
(OUT/'native-verification.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
