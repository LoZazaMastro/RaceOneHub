"""Build and check the generated-game bootstrap; never launch the game."""
from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import hashlib
import json
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
VC = Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK = Path(r'C:\Program Files (x86)\Windows Kits\10')
VERSION = '10.0.26100.0'
BUILD = ROOT / 'analysis/pc-build/bootstrap'
OUT = ROOT / 'runtime/builder/ForzaMastro5.exe'


def compile_c(source, output, extra=(), subsystem='CONSOLE'):
    command = [str(VC / 'bin/Hostx64/x64/cl.exe'), '/nologo', '/W4', '/WX',
               '/O2', '/MT', '/DUNICODE', '/D_UNICODE', '/guard:cf',
               '/I' + str(VC / 'include')]
    command += ['/I' + str(SDK / 'Include' / VERSION / part) for part in ('ucrt', 'shared', 'um')]
    command += [str(source), '/Fo' + str(BUILD) + '\\', '/Fe' + str(output)]
    command += list(extra)
    command += ['/link', '/WX', '/SUBSYSTEM:' + subsystem, '/Brepro', '/DYNAMICBASE', '/NXCOMPAT',
                '/LIBPATH:' + str(VC / 'lib/x64'),
                '/LIBPATH:' + str(SDK / 'Lib' / VERSION / 'ucrt/x64'),
                '/LIBPATH:' + str(SDK / 'Lib' / VERSION / 'um/x64'),
                'kernel32.lib', 'user32.lib', 'shell32.lib']
    result = subprocess.run(command, cwd=BUILD, capture_output=True, text=True)
    (BUILD / (output.stem + '-compiler.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return command


def main():
    BUILD.mkdir(parents=True, exist_ok=True)
    OUT.parent.mkdir(parents=True, exist_ok=True)
    resource = BUILD / 'bootstrap.rc'
    resource.write_text('1 RCDATA { 0 }\n', encoding='ascii')
    rc = subprocess.run([str(SDK / 'bin' / VERSION / 'x64/rc.exe'), '/nologo',
                         '/fo', str(BUILD / 'bootstrap.res'), str(resource)],
                        cwd=BUILD, capture_output=True, text=True)
    (BUILD / 'resource-compiler.log').write_text(rc.stdout + rc.stderr, encoding='utf-8')
    rc.check_returncode()
    command = compile_c(ROOT / 'src/fm5_bootstrap.c', OUT,
                        [str(BUILD / 'bootstrap.res')], subsystem='WINDOWS')
    add_python_libraries(__file__)
    from pc_icon_resources import embed_icon
    import pefile
    embed_icon(OUT, ROOT / 'launcher/ForzaMastro.Launcher/Assets/ForzaMastro.ico')
    pe = pefile.PE(str(OUT))
    resource_types = [entry.id for entry in pe.DIRECTORY_ENTRY_RESOURCE.entries]
    pe.close()
    assert 3 in resource_types and 14 in resource_types

    fixture = BUILD / 'environment_fixture.c'
    include = (ROOT / 'src/fm5_bootstrap.c').as_posix()
    fixture.write_text('#include "' + include + '"\n' + r'''
#include <assert.h>
int main(void) {
    WCHAR *block, *entry;
    BOOL normal = FALSE, unicode = FALSE;
    assert(SetEnvironmentVariableW(L"FM5_TEST_PROBE", L"1"));
    assert(SetEnvironmentVariableW(L"fm5_diagnostic", L"1"));
    assert(SetEnvironmentVariableW(L"BOOTSTRAP_NORMAL", L"keep"));
    assert(SetEnvironmentVariableW(L"BOOTSTRAP_UNICODE", L"\x00e8\x65e5"));
    block = fm5_clean_environment(); assert(block);
    for (entry = block; *entry; entry += wcslen(entry) + 1) {
        assert(_wcsnicmp(entry, L"FM5_", 4) != 0);
        if (!wcscmp(entry, L"BOOTSTRAP_NORMAL=keep")) normal = TRUE;
        if (!wcscmp(entry, L"BOOTSTRAP_UNICODE=\x00e8\x65e5")) unicode = TRUE;
    }
    assert(normal && unicode && entry[0] == 0);
    HeapFree(GetProcessHeap(), 0, block);
    return 0;
}
''', encoding='utf-8')
    fixture_exe = BUILD / 'environment_fixture.exe'
    compile_c(fixture, fixture_exe)
    subprocess.run([str(fixture_exe)], cwd=BUILD, check=True, timeout=15)

    cases = []
    with tempfile.TemporaryDirectory(prefix='FM5 bootstrap spazi \u00e8 \u65e5 ') as temp:
        directory = Path(temp)
        bootstrap = directory / 'ForzaMastro5.exe'
        shutil.copy2(OUT, bootstrap)
        unrelated_cwd = directory / 'unrelated working directory'
        unrelated_cwd.mkdir()
        def check(name, expected):
            status = subprocess.run([str(bootstrap), '--check-layout'], cwd=unrelated_cwd,
                                    check=False, timeout=15).returncode
            assert status == expected, (name, status, expected)
            cases.append({'name': name, 'exit_code': status})
        check('missing runtime', 2)
        runtime = directory / 'runtime'; runtime.mkdir()
        (runtime / 'ForzaMastro5.exe').write_bytes(b'not executable: check-layout must never run this file')
        check('missing ini', 2)
        (runtime / 'data').mkdir()
        (runtime / 'data/RaceOneEngine.ini').write_text('[launcher]\n', encoding='utf-8')
        check('complete paths with spaces and unicode, unrelated cwd', 0)
        (directory / 'forzamastro.ini').write_text('[startup]\nshow_pre_game=1\n')
        check('enabled pre-game must exist', 2)
        (runtime / 'ForzaMastro5.PreGame.exe').write_bytes(b'not executable: check-layout must never launch')
        check('enabled pre-game complete', 0)
        (directory / 'forzamastro.ini').write_text('[startup]\nshow_pre_game=0\n')
        check('disabled pre-game remains direct', 0)
        (runtime / 'ForzaMastro5.exe').unlink()
        (runtime / 'ForzaMastro5.exe').mkdir()
        check('directory cannot substitute for executable', 2)
    result = {'command': command, 'exe': str(OUT), 'bytes': OUT.stat().st_size,
              'sha256': hashlib.sha256(OUT.read_bytes()).hexdigest(),
              'resource_types': resource_types, 'environment_fixture': 'passed',
              'layout_cases': cases, 'game_executed': False}
    (BUILD / 'result.json').write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
