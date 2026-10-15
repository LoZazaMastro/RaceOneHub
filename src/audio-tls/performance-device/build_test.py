from pathlib import Path
import hashlib,json,os,subprocess
HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
OUT=ROOT/'analysis/audio-tls/performance-device'
VS=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK=Path(r'C:\Program Files (x86)\Windows Kits\10');version=sorted((SDK/'Include').glob('*'),key=lambda p:tuple(map(int,p.name.split('.'))))[-1].name
cl=VS/'bin/Hostx64/x64/cl.exe';ml=cl.parent/'ml64.exe';env=os.environ.copy();env['PATH']=str(cl.parent)+os.pathsep+env['PATH']
env['INCLUDE']=';'.join(str(p) for p in [VS/'include',SDK/'Include'/version/'ucrt',SDK/'Include'/version/'shared',SDK/'Include'/version/'um'])
env['LIB']=';'.join(str(p) for p in [VS/'lib/x64',SDK/'Lib'/version/'ucrt/x64',SDK/'Lib'/version/'um/x64'])
subprocess.run([os.sys.executable,str(HERE/'generate_forwarders.py')],check=True)
subprocess.run([os.sys.executable,str(HERE/'make_fence_guard.py')],check=True)
assembler=[str(ml),'/nologo','/c','/Fo'+str(OUT/'device_forwarders.obj'),str(HERE/'device_forwarders.asm')]
p=subprocess.run(assembler,env=env,text=True,capture_output=True);(OUT/'asm-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
command=[str(cl),'/nologo','/W4','/WX','/O2','/MT','/DFM5_PERFORMANCE_TEST',str(HERE/'test_device.c'),str(HERE/'fm5_performance_device.c'),str(ROOT/'src/allocation-port/fm5_msaa4.c'),str(OUT/'device_forwarders.obj'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_device.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
p=subprocess.run(command,env=env,text=True,capture_output=True);(OUT/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(OUT/'test_device.exe')],text=True,capture_output=True,timeout=20);(OUT/'native-test.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
diagnostics=p.stderr.splitlines()
assert len(diagnostics)==5 and sum('old_slot=64 ' in line for line in diagnostics)==1,diagnostics
assert any('old_slot=64 ' in line and 'arg1=0x11223344 arg2=0x55667788 arg3=0x99aabbcc' in line for line in diagnostics),diagnostics
assert any('old_slot=56 ' in line and 'arg1=0xdeadbeef12345678' in line for line in diagnostics),diagnostics
production=[str(cl),'/nologo','/W4','/WX','/O2','/MT','/c',str(HERE/'fm5_performance_device.c'),'/Fo'+str(OUT/'fm5_performance_device.production.obj')]
build=subprocess.run(production,env=env,text=True,capture_output=True);(OUT/'production-build.log').write_text(build.stdout+build.stderr);assert build.returncode==0,build.stdout+build.stderr
probe=[str(cl),'/nologo','/W4','/WX','/O2','/MT',str(HERE/'test_production_fence.c'),str(OUT/'fm5_performance_device.production.obj'),str(ROOT/'src/allocation-port/fm5_msaa4.c'),str(OUT/'device_forwarders.obj'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_production_fence.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
check=subprocess.run(probe,env=env,text=True,capture_output=True);(OUT/'production-fence-build.log').write_text(check.stdout+check.stderr);assert check.returncode==0,check.stdout+check.stderr
check=subprocess.run([str(OUT/'test_production_fence.exe')],text=True,capture_output=True,timeout=20);(OUT/'production-fence.log').write_text(check.stdout+check.stderr);assert check.returncode==0,(check.returncode,check.stdout,check.stderr)
production_fence=json.loads(check.stdout)
report=json.loads(p.stdout);report['command']=command;report['assembler_command']=assembler;report['sources']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in HERE.glob('*') if p.suffix in ['.c','.h','.asm']}
report['fatal_diagnostics']=diagnostics
report['production_compile_command']=production
report['production_fence_guard']=production_fence
report['limitation']='Slots3-6 route to external legacy resource adapter, slot7 to FMASK-aware SRV adapter, old56 to real native60 fence polling, old58 to genuine placement buffer adapter, old60/61 to genuine placement Texture2D/3D adapters, context output slots use external context cache, remaining first53 slots use same-slot taildelegation. Optional54 modes except4 and other extended slots fail closed. Context/resource adapters are recorders in this test; no game or GPU execution.'
(OUT/'native-verification.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
