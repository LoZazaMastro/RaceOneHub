from pathlib import Path
import hashlib,json,os,subprocess,sys
HERE=Path(__file__).resolve().parent;ROOT=HERE.parents[2]
exec((ROOT/'src/audio-tls/resource-port/build_test.py').read_text().split("subprocess.run([sys.executable")[0])
HERE=ROOT/'src/audio-tls/input-port';OUT=ROOT/'analysis/audio-tls/input-port';OUT.mkdir(parents=True,exist_ok=True)
env['INCLUDE']+=';'+str(SDK/'Include'/version/'winrt')
common=[str(cl),'/nologo','/W4','/WX','/O2','/MT']
impulse=[str(ROOT/'src/fm5_impulse_triggers.c'),str(ROOT/'src/fm5_impulse_wgi.cpp')]
commands=[common+['/DFM5_GAMEPAD_DIAGNOSTICS_TEST','/DFM5_IMPULSE_TRIGGERS_TEST',str(HERE/'test_gamepad_diagnostics.c'),str(ROOT/'src/fm5_gamepad_diagnostics.c'),str(ROOT/'src/fm5_impulse_triggers.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_gamepad_diagnostics.exe'),'/link','/DYNAMICBASE','/NXCOMPAT','user32.lib'],
          common+[str(HERE/'test_gamepad_production.c'),str(ROOT/'src/fm5_gamepad_diagnostics.c'),*impulse,'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_gamepad_production.exe'),'/link','/DYNAMICBASE','/NXCOMPAT','user32.lib'],
          common+['/DFM5_IMPULSE_TRIGGERS_TEST',str(HERE/'test_impulse_triggers.c'),str(ROOT/'src/fm5_impulse_triggers.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'test_impulse_triggers.exe'),'/link','/DYNAMICBASE','/NXCOMPAT'],
          common+[str(HERE/'probe_impulse_triggers.c'),*impulse,'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'probe_impulse_triggers.exe'),'/link','/DYNAMICBASE','/NXCOMPAT']]
for i,cmd in enumerate(commands):
 p=subprocess.run(cmd,env=env,text=True,capture_output=True);(OUT/f'diagnostic-build-{i}.log').write_text(p.stdout+p.stderr);assert p.returncode==0,p.stdout+p.stderr
results=[]
for enabled,forwarding in [(False,True),(True,True),(False,False),(True,False)]:
 testenv=env.copy();testenv['FM5_GAMEPAD_DIAGNOSTICS']='1'if enabled else'0';testenv['FM5_IMPULSE_TRIGGERS']='1'if forwarding else'0'
 p=subprocess.run([str(OUT/'test_gamepad_diagnostics.exe')],env=testenv,text=True,capture_output=True,timeout=20);(OUT/f'diagnostic-test-{enabled}-{forwarding}.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
 lines=p.stderr.splitlines();assert len(lines)==(128 if enabled else 0),len(lines)
 if enabled:assert all(l.startswith('FM5_GAMEPAD_VIBRATION ')for l in lines)and 'mask634=0x1 mask638=0x1234'in lines[0]and 'pre_cache=0x00000000 post_cache=0x12345678'in lines[0]
 result=json.loads(p.stdout);assert result['forwarding']==forwarding,result;results.append(result)
p=subprocess.run([str(OUT/'test_gamepad_production.exe')],text=True,capture_output=True,timeout=20);assert p.returncode==0,(p.returncode,p.stdout,p.stderr);results.append(json.loads(p.stdout))
p=subprocess.run([str(OUT/'test_impulse_triggers.exe')],text=True,capture_output=True,timeout=20);(OUT/'impulse-test.log').write_text(p.stdout+p.stderr);assert p.returncode==0 and not p.stderr,(p.returncode,p.stdout,p.stderr);results.append(json.loads(p.stdout))
p=subprocess.run([str(OUT/'test_impulse_triggers.exe'),'--log'],text=True,capture_output=True,timeout=20);(OUT/'impulse-log-test.log').write_text(p.stdout+p.stderr);assert p.returncode==0,(p.returncode,p.stdout,p.stderr)
lines=p.stderr.splitlines();assert len(lines)==17 and all(l.startswith('FM5_IMPULSE_TRIGGERS seq=')for l in lines),lines
assert 'index=0 pads=1'in lines[0] and 'motors=[0.000,0.000] triggers=[0.250,0.750] gate=1 hr=0x00000000'in lines[0],lines[0]
results.append(dict(json.loads(p.stdout),diagnostic_lines=len(lines)))
sources=[ROOT/'src/fm5_gamepad_diagnostics.c',ROOT/'src/fm5_gamepad_diagnostics.h',ROOT/'src/fm5_impulse_triggers.c',ROOT/'src/fm5_impulse_triggers.h',ROOT/'src/fm5_impulse_wgi.cpp',ROOT/'src/fm5_bridge_main.c']
report={'passed':True,'tests':results,'maximum_diagnostic_lines':128,'sources':{p.name:hashlib.sha256(p.read_bytes()).hexdigest()for p in sources},'commands':commands,'scope':'Native C ABI recorders, input immutability, unchanged backend result and gate state; diagnostics disabled silence, enabled rate/128 budget, production wrong-host guard; vtable slot install; impulse trigger forwarding, gates, retries and pad matching against a fake device. probe_impulse_triggers.exe is built, not run: it reads real controllers. No controller output, game process, or runtime writes.'}
(OUT/'diagnostic-verification.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
