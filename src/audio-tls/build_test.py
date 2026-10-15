from pathlib import Path
import hashlib
import json
import os
import subprocess
import sys
import ctypes
ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
OUT = ROOT / 'analysis/audio-tls'
VS = Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK = Path(r'C:\Program Files (x86)\Windows Kits\10')
sdk_version = sorted((SDK / 'Include').glob('*'), key=lambda p: tuple(map(int,p.name.split('.'))))[-1].name
compiler = VS / 'bin/Hostx64/x64/cl.exe'
env = os.environ.copy()
env['PATH'] = str(compiler.parent) + os.pathsep + env['PATH']
env['INCLUDE'] = ';'.join(str(p) for p in [VS / 'include', SDK / 'Include' / sdk_version / 'ucrt', SDK / 'Include' / sdk_version / 'shared', SDK / 'Include' / sdk_version / 'um'])
env['LIB'] = ';'.join(str(p) for p in [VS / 'lib/x64', SDK / 'Lib' / sdk_version / 'ucrt/x64', SDK / 'Lib' / sdk_version / 'um/x64'])
command = [str(compiler), '/nologo', '/W4', '/WX', '/O2', '/MT', '/DUNICODE', '/D_UNICODE', str(HERE / 'tls_probe.c'), '/Fo' + str(OUT / 'tls_probe.obj'), '/Fe' + str(OUT / 'tls_probe.exe'), '/link', '/DYNAMICBASE', '/NXCOMPAT']
result = subprocess.run(command, env=env, capture_output=True, text=True)
(OUT / 'build.log').write_text(result.stdout + result.stderr)
assert result.returncode == 0, result.stdout + result.stderr
helper = ROOT / 'runtime/azimuth-0.9.9-original/data/bin/azimuth-tls.dll'
assert hashlib.sha256(helper.read_bytes()).hexdigest() == 'ffa229b6c62228bf6126de6f140c6a70326c536524bec00f9304ac6591805bb7'
probe = subprocess.run([str(OUT / 'tls_probe.exe'), str(helper)], capture_output=True, text=True, timeout=20)
(OUT / 'probe.log').write_text(probe.stdout + probe.stderr)
assert probe.returncode == 0, (probe.returncode, probe.stdout, probe.stderr)
report = json.loads(probe.stdout)
report.update({'scope': 'Native Windows OS static TLS allocation and five-thread isolation. Does not execute the game.', 'command': command,
               'official_helper_sha256': hashlib.sha256(helper.read_bytes()).hexdigest(),
               'source_sha256': hashlib.sha256((HERE / 'tls_probe.c').read_bytes()).hexdigest()})
(OUT / 'tls-native-verification.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
command = [str(compiler), '/nologo', '/W4', '/WX', '/O2', '/MT', '/DFM5_BRIDGE_TEST', str(HERE / 'audio_probe.c'), str(HERE / 'fm5_audio_bridge.c'), '/Fo' + str(OUT) + '\\', '/Fe' + str(OUT / 'audio_probe.exe'), '/link', '/DYNAMICBASE', '/NXCOMPAT']
result = subprocess.run(command, env=env, capture_output=True, text=True)
(OUT / 'audio-build.log').write_text(result.stdout + result.stderr)
assert result.returncode == 0, result.stdout + result.stderr
probe = subprocess.run([str(OUT / 'audio_probe.exe')], capture_output=True, text=True, timeout=20)
(OUT / 'audio-probe.log').write_text(probe.stdout + probe.stderr)
assert probe.returncode == 0, (probe.returncode, probe.stdout, probe.stderr)
report = json.loads(probe.stdout)
report.update({'scope': 'Real x64 ABI adapter compiled with recorder backend. Azimuth audio engine and game not executed.', 'command': command})
(OUT / 'audio-native-verification.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
command = [str(compiler), '/nologo', '/W4', '/WX', '/O2', '/MT', '/LD', str(HERE / 'fm5_audio_bridge.c'), '/Fo' + str(OUT / 'fm5_audio_bridge_production.obj'), '/Fe' + str(OUT / 'fm5_audio_bridge_probe.dll'), '/link', '/DYNAMICBASE', '/NXCOMPAT']
result = subprocess.run(command, env=env, capture_output=True, text=True)
(OUT / 'audio-production-build.log').write_text(result.stdout + result.stderr)
assert result.returncode == 0, result.stdout + result.stderr
dll = ctypes.WinDLL(str(OUT / 'fm5_audio_bridge_probe.dll'))
create = dll.FM5XAudio2Create
create.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_uint32, ctypes.c_uint32, ctypes.c_void_p]
create.restype = ctypes.c_int32
output = ctypes.c_void_p(1)
hr = create(ctypes.byref(output), 0, 0x400, None)
assert ctypes.c_uint32(hr).value == 0x800700c1 and output.value is None
(OUT / 'audio-production-host-rejection.json').write_text(json.dumps({'passed':True,
    'scope':'Production DLL loaded in Python process rejects wrong host before any Azimuth call.',
    'hresult':hex(ctypes.c_uint32(hr).value),'output_is_null':True},indent=2))
