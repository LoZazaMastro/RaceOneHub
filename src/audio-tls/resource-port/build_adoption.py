from pathlib import Path
import hashlib,json,os,subprocess,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
exec((HERE/'build_test.py').read_text().split("subprocess.run([sys.executable")[0])
OUT=ROOT/'analysis/audio-tls/resource-port/adoption';OUT.mkdir(parents=True,exist_ok=True)
command=[str(cl),'/nologo','/W4','/WX','/O2','/MT','/DFM5_RESOURCE_TEST','/DFM5_ALLOCATION_TEST','/DFM5_DXGI_TEST',str(HERE/'test_swapchain_adoption.c'),str(HERE/'fm5_resource.c'),str(ROOT/'src/graphics-port/fm5_dxgi_bridge.c'),str(ROOT/'src/allocation-port/fm5_allocation.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_swapchain_adoption.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']
p=subprocess.run(command,env=env,text=True,capture_output=True);(OUT/'build.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
p=subprocess.run([str(OUT/'test_swapchain_adoption.exe')],text=True,capture_output=True,timeout=20);(OUT/'test.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
report=json.loads(p.stdout);report['compile_command']=command
report['sources']={str(p.relative_to(ROOT)):hashlib.sha256(p.read_bytes()).hexdigest() for p in [HERE/'fm5_resource.c',HERE/'fm5_resource.h',ROOT/'src/graphics-port/fm5_dxgi_bridge.c',ROOT/'src/graphics-port/fm5_dxgi_bridge.h',HERE/'test_swapchain_adoption.c']}
(OUT/'native-verification.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
