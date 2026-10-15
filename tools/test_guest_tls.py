from pathlib import Path
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[1]
VC = Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK = Path(r'C:\Program Files (x86)\Windows Kits\10')
VERSION = '10.0.26100.0'
OUT = ROOT / 'analysis/pc-build'
common = [str(VC / 'bin/Hostx64/x64/cl.exe'), '/nologo', '/W4', '/WX', '/O2', '/MT',
          '/I' + str(VC / 'include')]
common += ['/I' + str(SDK / 'Include' / VERSION / p) for p in ['ucrt', 'shared', 'um']]
link = ['/link', '/Brepro', '/LIBPATH:' + str(VC / 'lib/x64')]
link += ['/LIBPATH:' + str(SDK / 'Lib' / VERSION / p / 'x64') for p in ['ucrt', 'um']]
link += ['kernel32.lib']
source = ROOT / 'src/test_fm5_guest_tls.c'
commands = [common + ['/LD', '/DFM5_TLS_FIXTURE', str(source), '/Fo' + str(OUT / 'tls_fixture.obj'),
                      '/Fe' + str(OUT / 'tls_fixture.dll')] + link,
            common + [str(source), str(ROOT / 'src/fm5_guest_tls.c'), '/Fo' + str(OUT) + '\\',
                      '/Fe' + str(OUT / 'test_guest_tls.exe')] + link]
for command in commands:
    run = subprocess.run(command, text=True, capture_output=True, cwd=OUT)
    if run.returncode:
        raise SystemExit(run.stdout + run.stderr)
run = subprocess.run([str(OUT / 'test_guest_tls.exe'), str(OUT / 'tls_fixture.dll')],
                     text=True, capture_output=True, cwd=OUT, timeout=20)
(OUT / 'guest-tls-test.json').write_text(json.dumps({
    'passed': run.returncode == 0, 'stdout': run.stdout, 'stderr': run.stderr,
    'exit_code': run.returncode, 'commands': commands,
    'scope': 'Native CPU execution of both TLS-register thunks on two Windows threads. Flags, unrelated register and host TLS storage preservation. Read/execute protection. No game execution.',
    'source_sha256': hashlib.sha256((ROOT / 'src/fm5_guest_tls.c').read_bytes()).hexdigest()
}, indent=2) + '\n')
if run.returncode:
    raise SystemExit(run.stdout + run.stderr + '\nexit=' + str(run.returncode))
print(run.stdout, end='')
