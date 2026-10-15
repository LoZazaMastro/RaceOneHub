from pathlib import Path
import hashlib,json,os,subprocess
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
OUT=ROOT/'analysis/audio-tls/private-data';OUT.mkdir(parents=True,exist_ok=True)
VS=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK=Path(r'C:\Program Files (x86)\Windows Kits\10');version=sorted((SDK/'Include').glob('*'),key=lambda p:tuple(map(int,p.name.split('.'))))[-1].name
cl=VS/'bin/Hostx64/x64/cl.exe';env=os.environ.copy();env['PATH']=str(cl.parent)+os.pathsep+env['PATH']
env['INCLUDE']=';'.join(str(p) for p in [VS/'include',SDK/'Include'/version/'ucrt',SDK/'Include'/version/'shared',SDK/'Include'/version/'um'])
env['LIB']=';'.join(str(p) for p in [VS/'lib/x64',SDK/'Lib'/version/'ucrt/x64',SDK/'Lib'/version/'um/x64'])
common=[str(cl),'/nologo','/W4','/WX','/O2','/MT']
command=common+['/DFM5_PRIVATE_DATA_TEST',str(HERE/'test_private_data.c'),str(HERE/'fm5_private_data.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_private_data.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
p=subprocess.run(command,env=env,text=True,capture_output=True);(OUT/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
production=common+['/c',str(HERE/'fm5_private_data.c'),'/Fo'+str(OUT/'fm5_private_data.production.obj')]
p=subprocess.run(production,env=env,text=True,capture_output=True);(OUT/'production-build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(OUT/'test_private_data.exe')],text=True,capture_output=True,timeout=25);(OUT/'native-test.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
report=json.loads(p.stdout);report['compile_command']=command;report['production_command']=production
report['sources']={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in HERE.glob('*') if p.suffix in ['.c','.h']}
(OUT/'native-verification.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
