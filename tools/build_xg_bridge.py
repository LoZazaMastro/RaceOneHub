from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
VC = Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK = Path(r'C:\Program Files (x86)\Windows Kits\10')
VERSION = '10.0.26100.0'
OUT = ROOT / 'runtime/xg-host'
OUT.mkdir(parents=True, exist_ok=True)
command = [str(VC / 'bin/Hostx64/x64/cl.exe'), '/nologo', '/W4', '/WX', '/O2', '/MT', '/LD',
           '/I' + str(VC / 'include')]
command += ['/I' + str(SDK / 'Include' / VERSION / part) for part in ['ucrt', 'shared', 'um']]
command += [str(ROOT / 'src/fm5_xmem.c'), '/Fo' + str(OUT) + '\\',
            '/Fe' + str(OUT / 'kernelx.dll'), '/link', '/Brepro',
            '/DEF:' + str(ROOT / 'src/kernelx-forwarders.def'),
            '/LIBPATH:' + str(VC / 'lib/x64')]
command += ['/LIBPATH:' + str(SDK / 'Lib' / VERSION / part / 'x64') for part in ['ucrt', 'um']]
command += ['kernel32.lib']
result = subprocess.run(command, text=True, capture_output=True)
(ROOT / 'analysis/xg-build.log').write_text(result.stdout + result.stderr)
assert result.returncode == 0, result.stdout + result.stderr
for name in ['xg.dll', 'xg_x.dll']:
    source = ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32' / name
    shutil.copy2(source, OUT / name)
modern = ROOT / 'analysis/xg-modern/xg_x.dll'
assert hashlib.sha256(modern.read_bytes()).hexdigest() == '9e3a06d02784699c252bea271090b9e2a04ef2cb29b5b0647c6ce6e73b19db67'
shutil.copyfile(modern, OUT / 'fm5-host-xg.dll')
report = {'command': command, 'exit_code': result.returncode,
          'scope': 'Private standard Win32 forwarders and real ERA heap XMem allocation. Original FM5 XG for guest; pinned modern8-slot XG for Azimuth host only. Unsupported nonheap XMem attributes fail explicitly.',
          'files': [{'path': str(p), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest(), 'bytes': p.stat().st_size}
                    for p in OUT.glob('*.dll')]}
(ROOT / 'analysis/xg-build.json').write_text(json.dumps(report, indent=2) + '\n')
test = subprocess.run([sys.executable, '-X', 'utf8', '-B', str(ROOT / 'tools/test_xg_load.py')],
                      text=True, capture_output=True, timeout=20)
(ROOT / 'analysis/xg-load.log').write_text(test.stdout + test.stderr)
assert test.returncode == 0, test.stdout + test.stderr
print(test.stdout)
