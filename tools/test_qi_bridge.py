from pathlib import Path
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[1]
VC = Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK = Path(r'C:\Program Files (x86)\Windows Kits\10')
VERSION = '10.0.26100.0'
OUT = ROOT / 'analysis/pc-build'
SOURCES = [ROOT / 'src/test_fm5_qi_bridge.c', ROOT / 'src/fm5_qi_bridge.c']
command = [str(VC / 'bin/Hostx64/x64/cl.exe'), '/nologo', '/W4', '/WX', '/O2', '/MT',
           '/DFM5_QI_BRIDGE_TEST', '/I' + str(VC / 'include')]
command += ['/I' + str(SDK / 'Include' / VERSION / part) for part in ['ucrt', 'shared', 'um']]
command += [str(p) for p in SOURCES]
command += ['/Fo' + str(OUT) + '\\', '/Fe' + str(OUT / 'test_fm5_qi_bridge.exe'), '/link', '/Brepro',
            '/LIBPATH:' + str(VC / 'lib/x64')]
command += ['/LIBPATH:' + str(SDK / 'Lib' / VERSION / part) for part in ['ucrt/x64', 'um/x64']]
command += ['kernel32.lib']
build = subprocess.run(command, text=True, capture_output=True, cwd=OUT)
(OUT / 'qi-compiler.log').write_text(build.stdout + build.stderr)
build.check_returncode()
test = subprocess.run([str(OUT / 'test_fm5_qi_bridge.exe')], text=True, capture_output=True, cwd=OUT)
(OUT / 'qi-test.log').write_text(test.stdout + test.stderr)
test.check_returncode()
(OUT / 'qi-test.json').write_text(json.dumps({
    'compiler_exit_code': build.returncode, 'test_exit_code': test.returncode,
    'command': command, 'output': test.stdout,
    'sources': [{'path': str(p.relative_to(ROOT)), 'sha256': hashlib.sha256(p.read_bytes()).hexdigest()}
                for p in SOURCES],
    'scope': 'Native mocked QI delegation and reference ownership; no guest or graphics API execution.'
}, indent=2) + '\n')
print(test.stdout, end='')
