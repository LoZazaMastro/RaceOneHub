from pathlib import Path
import subprocess,json,hashlib
R=Path(__file__).resolve().parents[2];O=R/'analysis/allocation-port'
VC=Path(r'C:/Program Files/Microsoft Visual Studio/2022/Community/VC/Tools/MSVC/14.44.35207');SDK=Path(r'C:/Program Files (x86)/Windows Kits/10');V='10.0.26100.0'
S=[R/'src/allocation-port/fm5_allocation.c',R/'src/allocation-port/test_fm5_allocation.c']
C=[str(VC/'bin/Hostx64/x64/cl.exe'),'/nologo','/W4','/WX','/O2','/MT','/DFM5_ALLOCATION_TEST','/I'+str(VC/'include')]
C+=['/I'+str(SDK/'Include'/V/p)for p in ['ucrt','shared','um']]+[str(p)for p in S]
C+=['/Fo'+str(O)+'\\','/Fe'+str(O/'test_fm5_allocation.exe'),'/link','/Brepro','/LIBPATH:'+str(VC/'lib/x64')]+['/LIBPATH:'+str(SDK/'Lib'/V/p)for p in ['ucrt/x64','um/x64']]+['kernel32.lib']
b=subprocess.run(C,capture_output=True,text=True,cwd=O);(O/'compiler.log').write_text(b.stdout+b.stderr);b.check_returncode()
t=subprocess.run([str(O/'test_fm5_allocation.exe')],capture_output=True,text=True,cwd=O);(O/'test.log').write_text(t.stdout+t.stderr);t.check_returncode()
(O/'test.json').write_text(json.dumps({'compiler_exit_code':b.returncode,'test_exit_code':t.returncode,'stdout':t.stdout,'command':C,'sources':[{'path':str(p.relative_to(R)),'sha256':hashlib.sha256(p.read_bytes()).hexdigest()}for p in S],'scope':'Native mock ABI verification only; no game, GPU, PS5 or FTP execution.'},indent=2)+'\n');print(t.stdout)
