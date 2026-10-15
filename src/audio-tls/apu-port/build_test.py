from pathlib import Path
import hashlib,json,os,subprocess,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
exec((ROOT/'src/audio-tls/resource-port/build_test.py').read_text().split("subprocess.run([sys.executable")[0])
HERE=ROOT/'src/audio-tls/apu-port';OUT=ROOT/'analysis/audio-tls/apu-port';OUT.mkdir(parents=True,exist_ok=True)
common=[str(cl),'/nologo','/W4','/WX','/O2','/MT']
commands=[common+['/DFM5_APU_TEST',str(HERE/'test_apu.c'),str(HERE/'fm5_apu.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_apu.exe'),'/link','/DYNAMICBASE','/NXCOMPAT'],common+[str(HERE/'test_production.c'),str(HERE/'fm5_apu.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_production.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']]
results=[]
for i,cmd in enumerate(commands):
 p=subprocess.run(cmd,env=env,text=True,capture_output=True);(OUT/f'build-{i}.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
 exe=OUT/('test_apu.exe'if i==0 else'test_production.exe');p=subprocess.run([str(exe)],text=True,capture_output=True,timeout=20);(OUT/f'test-{i}.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(p.returncode,p.stdout,p.stderr);results.append(json.loads(p.stdout))
report={'tests':results,'compile_commands':commands,'sources':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in HERE.glob('*')if p.suffix in ['.c','.h']},'scope':'Native Windows typed ABI recorder tests, real VirtualAlloc storage in recorder, no game or genuine Azimuth heap execution'}
(OUT/'native-verification.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
