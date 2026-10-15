from pathlib import Path
import os,subprocess,json,hashlib
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[1]
OUT=ROOT/'analysis/ui-port/native';OUT.mkdir(exist_ok=True)
VS=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK=Path(r'C:\Program Files (x86)\Windows Kits\10');v=sorted((SDK/'Include').glob('*'),key=lambda p:tuple(map(int,p.name.split('.'))))[-1].name
cl=VS/'bin/Hostx64/x64/cl.exe';env=os.environ.copy();env['PATH']=str(cl.parent)+os.pathsep+env['PATH']
env['INCLUDE']=';'.join(str(p) for p in [VS/'include',SDK/'Include'/v/'ucrt',SDK/'Include'/v/'shared',SDK/'Include'/v/'um'])
env['LIB']=';'.join(str(p) for p in [VS/'lib/x64',SDK/'Lib'/v/'ucrt/x64',SDK/'Lib'/v/'um/x64'])
cmd=[str(cl),'/nologo','/W4','/WX','/O2','/MT','/DFM5_NATIVE_UI_TEST',str(HERE/'fm5_native_ui.c'),str(HERE/'fm5_native_loading.c'),str(HERE/'test_native_ui.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_native_ui.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
p=subprocess.run(cmd,env=env,capture_output=True,text=True);(OUT/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
for source in ['fm5_native_ui.c','fm5_native_binding.c']:
 p=subprocess.run([str(cl),'/nologo','/W4','/WX','/O2','/MT','/c',str(HERE/source),'/Fo'+str(OUT/(source+'.obj'))],env=env,capture_output=True,text=True);(OUT/(source+'.build.log')).write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(OUT/'test_native_ui.exe'),str(ROOT/'pregame/assets/fm5-background-clean.png'),str(OUT/'settings-preview.bmp')],capture_output=True,text=True,timeout=30);assert p.returncode==0,p.stdout+p.stderr
r=json.loads(p.stdout);r['sources']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in HERE.glob('*.[ch]')};(OUT/'test-report.json').write_text(json.dumps(r,indent=2));print(json.dumps(r,indent=2))
cmd=[str(cl),'/nologo','/W4','/WX','/O2','/MT',str(HERE/'fm5_native_ui.c'),str(HERE/'fm5_native_loading.c'),str(HERE/'test_native_binding.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_native_binding.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
p=subprocess.run(cmd,env=env,capture_output=True,text=True);(OUT/'binding-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(OUT/'test_native_binding.exe')],capture_output=True,text=True,timeout=20);assert p.returncode==0,p.stdout+p.stderr
(OUT/'binding-test-report.json').write_text(p.stdout);print(p.stdout)
cmd=[str(cl),'/nologo','/W4','/WX','/O2','/MT','/DFM5_NATIVE_UI_TEST',str(HERE/'fm5_native_loading.c'),str(HERE/'test_native_loading.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_native_loading.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
p=subprocess.run(cmd,env=env,capture_output=True,text=True);(OUT/'loading-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(OUT/'test_native_loading.exe'),str(ROOT/'pregame/assets/fm5-background-clean.png'),str(OUT/'loading-preview.bmp')],capture_output=True,text=True,timeout=30);assert p.returncode==0,p.stdout+p.stderr
(OUT/'loading-test-report.json').write_text(p.stdout);print(p.stdout)
