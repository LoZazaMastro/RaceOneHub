"""Prepare a private FM5 runtime from explicit user inputs. Never start a game."""
from __future__ import annotations
import argparse
import hashlib
import importlib.util
import json
import os
from pathlib import Path
import shutil
import stat
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def digest(path):
    value = hashlib.sha256()
    with Path(path).open('rb') as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b''):
            value.update(block)
    return value.hexdigest()


def plain(path, *, directory=False):
    path = Path(os.path.abspath(path))
    for ancestor in (path, *path.parents):
        if not ancestor.exists():
            continue
        info = ancestor.lstat()
        if stat.S_ISLNK(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('Use a real local path without links or reparse points: ' + str(path))
    if directory and not path.is_dir() or not directory and not path.is_file():
        raise ValueError('Required input is missing: ' + str(path))
    return path


def checked_input(path, expected):
    path = plain(path)
    if path.stat().st_size != expected['bytes'] or digest(path) != expected['sha256']:
        raise ValueError('Unsupported input (exact size/hash differs): ' + str(path))
    return path


def toolchain(vc_dir=None, sdk_dir=None, sdk_version=None):
    if vc_dir is None:
        finder = Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Microsoft Visual Studio/Installer/vswhere.exe'
        if not finder.is_file():
            raise ValueError('Install Visual Studio C++ Build Tools, or supply --vc-dir.')
        result = subprocess.run([str(finder), '-latest', '-products', '*', '-requires',
            'Microsoft.VisualStudio.Component.VC.Tools.x86.x64', '-property', 'installationPath'],
            capture_output=True, text=True, check=True)
        candidates = list((Path(result.stdout.strip()) / 'VC/Tools/MSVC').glob('*'))
        candidates = [p for p in candidates if (p/'bin/Hostx64/x64/cl.exe').is_file()]
        if not candidates:
            raise ValueError('An x64 Visual Studio C++ toolchain is required.')
        vc_dir = max(candidates, key=lambda p: tuple(map(int, p.name.split('.'))))
    vc_dir = plain(vc_dir, directory=True)
    sdk_dir = plain(sdk_dir or Path(os.environ.get('ProgramFiles(x86)', r'C:\Program Files (x86)')) / 'Windows Kits/10', directory=True)
    if sdk_version is None:
        versions = [p.name for p in (sdk_dir/'Include').glob('*') if (p/'um/windows.h').is_file()]
        if not versions:
            raise ValueError('Install the Windows 10/11 SDK C++ headers and x64 libraries.')
        sdk_version = max(versions, key=lambda v: tuple(map(int, v.split('.'))))
    for name in ['cl.exe', 'ml64.exe']:
        plain(vc_dir/'bin/Hostx64/x64'/name)
    for path in [vc_dir/'include/vcruntime.h', vc_dir/'lib/x64/libcmt.lib',
                 sdk_dir/'Include'/sdk_version/'um/windows.h', sdk_dir/'Lib'/sdk_version/'um/x64/kernel32.lib']:
        plain(path)
    return vc_dir, sdk_dir, sdk_version


def run(command, source, name):
    directory = source/'analysis/preparation'
    directory.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment['PYTHONPATH'] = os.pathsep.join([str(source/'tools'), *sys.path])
    print('Preparing: ' + name, flush=True)
    result = subprocess.run(list(map(str, command)), cwd=source, env=environment,
                            capture_output=True, text=True)
    (directory/(name+'.log')).write_text(result.stdout + result.stderr, encoding='utf-8')
    if result.returncode:
        raise RuntimeError(name + ' failed:\n' + (result.stdout + result.stderr)[-12000:])
    return result.stdout


def copy_authored_source(destination):
    # Only source roots; never copy ignored runtime directories or a whole checkout.
    allowed = {'.c', '.cpp', '.h', '.asm', '.inc', '.py', '.cmd', '.def', '.json', '.ps1'}
    private = {'fm5_language_guards.inc', 'fm5_resolve_original_guard.inc',
               'fm5_resolve_four_samples.inc', 'fm5_resolve_eight_samples.inc'}
    for folder in ['src', 'tools', 'recipes']:
        for path in (ROOT/folder).rglob('*'):
            if not path.is_file() or path.suffix not in allowed or path.name in private or '__pycache__' in path.parts:
                continue
            plain(path)
            target = destination/path.relative_to(ROOT)
            target.parent.mkdir(parents=True, exist_ok=True)
            shutil.copy2(path, target)
    target = destination/'launcher/ForzaMastro.Launcher/Assets'
    target.mkdir(parents=True)
    shutil.copy2(ROOT/'launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport5.ico', target)


def build_xg(source, game, modern, vc, sdk, version):
    out = source/'runtime/xg-host'
    out.mkdir(parents=True)
    command = [vc/'bin/Hostx64/x64/cl.exe', '/nologo', '/W4', '/WX', '/O2', '/MT', '/LD',
               '/I'+str(vc/'include')]
    command += ['/I'+str(sdk/'Include'/version/part) for part in ['ucrt', 'shared', 'um']]
    command += [source/'src/fm5_xmem.c', '/Fo'+str(out)+'\\', '/Fe'+str(out/'kernelx.dll'),
                '/link', '/Brepro', '/DEF:'+str(source/'src/kernelx-forwarders.def'), '/LIBPATH:'+str(vc/'lib/x64')]
    command += ['/LIBPATH:'+str(sdk/'Lib'/version/part/'x64') for part in ['ucrt', 'um']]
    command += ['kernel32.lib']
    run(command, source, 'xg-forwarder-build')
    for name in ['xg.dll', 'xg_x.dll']:
        shutil.copy2(game/'EmbeddedXvd/Windows/System32'/name, out/name)
    shutil.copy2(modern, out/'fm5-host-xg.dll')
    for extension in ['*.obj', '*.lib', '*.exp']:
        for path in out.glob(extension):
            path.unlink()  # Newly created, explicitly owned staging files only.


def validate_output(source):
    import pefile
    from fm5_background import validate_image
    runtime = source/'runtime'
    # pefile retains mmap handles through reference cycles. Close explicitly
    # before the Windows atomic directory rename and staging cleanup.
    with pefile.PE(str(runtime/'pc/ForzaMastro5.exe')) as host, \
         pefile.PE(str(runtime/'pc/ForzaMastro5Bridge.dll')) as bridge:
        needed = {i.name.decode() for d in host.DIRECTORY_ENTRY_IMPORT
                  if d.dll.lower() == b'forzamastro5bridge.dll' for i in d.imports}
        exports = {e.name.decode() for e in bridge.DIRECTORY_ENTRY_EXPORT.symbols if e.name}
    if not needed or not needed <= exports:
        raise RuntimeError('Prepared host and bridge export contract does not match.')
    if any(p.suffix.lower() in {'.obj', '.lib', '.exp', '.pdb', '.pipelines'} for p in runtime.rglob('*')):
        raise RuntimeError('Build debris/cache entered the prepared runtime.')
    # Load our newly compiled adapter only. No host or game executable is run.
    import ctypes
    kernel = ctypes.WinDLL('kernel32', use_last_error=True)
    kernel.LoadLibraryExW.argtypes = [ctypes.c_wchar_p, ctypes.c_void_p, ctypes.c_uint]
    kernel.LoadLibraryExW.restype = ctypes.c_void_p
    kernel.FreeLibrary.argtypes = [ctypes.c_void_p]
    module = kernel.LoadLibraryExW(str(runtime/'pc/ForzaMastro5Bridge.dll'), None, 0x1100)
    if not module:
        raise ctypes.WinError(ctypes.get_last_error())
    if not kernel.FreeLibrary(module):
        raise ctypes.WinError(ctypes.get_last_error())
    ui_exports = {'FM5RunPreGame','FM5DrawPreparation','FM5DrawPreparationComplete','FM5PaintOriginalPreparation'}
    if not ui_exports <= needed or not ui_exports <= exports:
        raise RuntimeError('Native pre-game/loading export closure is incomplete.')
    artwork = validate_image(runtime/'pc/data/ui/fm5-background-clean.png')
    required = ['pc/data/RaceOneEngine.ini','pc/data/licenses.txt',
        *('pc/data/bin/'+n for n in ['avcodec-62.dll','avutil-60.dll','azimuth-tls.dll','msvcp110.dll','msvcr110.dll']),
        *('xg-host/'+n for n in ['xg.dll','xg_x.dll','fm5-host-xg.dll','kernelx.dll'])]
    if not all((runtime/p).is_file() for p in required):
        raise RuntimeError('Prepared dependency copy closure is incomplete.')
    return {'host_bridge_exports_verified': sorted(needed), 'authored_bridge_load_passed': True,
            'native_pre_game_exports_verified':sorted(ui_exports),'background_decode':artwork,
            'dependency_files_verified':required,
            'game_executed': False, 'host_executed': False}


def prepare(engine_folder, game_folder, modern_xg, output, *, vc_dir=None, sdk_dir=None, sdk_version=None, shader_compiler=None, background=None):
    from fm5_game_files import validate_game_source
    from fm5_language_catalog import text_labels
    engine_folder = plain(engine_folder, directory=True)
    game = Path(validate_game_source(game_folder)['game_root'])
    catalog = json.loads((ROOT/'recipes/runtime-inputs.json').read_text('utf-8'))
    inputs = {}
    for name, expected in catalog['engine_files'].items():
        inputs[str(engine_folder/name)] = str(checked_input(engine_folder/name, expected))
    modern = checked_input(modern_xg, catalog['modern_xg'])
    background = plain(background or ROOT/'pregame/Assets/fm5-background-clean.png')
    if background.suffix.lower() != '.png' or background.read_bytes()[:8] != b'\x89PNG\r\n\x1a\n':
        raise ValueError('The pre-game background must be a PNG image.')
    from fm5_background import validate_image
    validate_image(background)
    for name, expected in catalog['game_sdk'].items():
        checked_input(game/'EmbeddedXvd/Windows/System32'/name, expected)
    vc, sdk, version = toolchain(vc_dir, sdk_dir, sdk_version)
    output = Path(os.path.abspath(output))
    if output.exists():
        raise ValueError('Choose a new runtime output directory; existing files are not replaced.')
    for input_root in [game, engine_folder, modern.parent]:
        if output == input_root or input_root in output.parents or output in input_root.parents:
            raise ValueError('Runtime output must be separate from every supplied input.')
    if ROOT in output.parents and output != ROOT/'runtime':
        raise ValueError('Use the default runtime directory or an output outside the source components.')
    output.parent.mkdir(parents=True, exist_ok=True)
    plain(output.parent, directory=True)
    with tempfile.TemporaryDirectory(prefix='.raceone-prepare-', dir=output.parent) as owned:
        stage = Path(owned)
        copy_authored_source(stage)
        generator_path = stage/'tools/generate_fm5_includes.py'
        spec = importlib.util.spec_from_file_location('raceone_owned_include_generator', generator_path)
        generator = importlib.util.module_from_spec(spec)
        sys.modules[spec.name] = generator
        spec.loader.exec_module(generator)
        includes = generator.generate(game/'Mount/forza_durango_release_final.exe',
                                      engine_folder/'azimuth.exe', stage/'generated-includes', compiler_path=shader_compiler)
        for generated in (stage/'generated-includes').glob('*.inc'):
            shutil.copy2(generated, stage/'src/allocation-port'/generated.name)
        run([sys.executable, '-B', stage/'tools/build_pc_bridge.py', '--graphics-adapters',
             '--vc-dir', vc, '--sdk-dir', sdk, '--sdk-version', version], stage, 'native-adapter-build')
        run([sys.executable, '-B', stage/'tools/build_pc_host.py', '--graphics-adapters',
             '--engine-folder', engine_folder, '--game-folder', game,
             '--preset', 'ultra', '--antialiasing', 'msaax4', '--frame-rate', '120'], stage, 'native-host-preparation')
        build_xg(stage, game, modern, vc, sdk, version)
        runtime = stage/'runtime'
        image = runtime/'pc/data/ui/fm5-background-clean.png'
        image.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(background, image)
        artwork = {'source': 'pre-game artwork; Microsoft and respective rights holders',
                   'input': str(background), 'sha256': digest(image)}
        verification = validate_output(stage)
        labels = text_labels(game)
        (runtime/'pc/data/forzamastro-languages.json').write_text(json.dumps(labels, indent=2)+'\n', encoding='utf-8')
        (runtime/'pc/forzamastro.ini').write_text('[startup]\nshow_pre_game=1\n', encoding='utf-8')
        report = {'schema': 1, 'prepared': True, 'scope': 'Private user-local runtime; never redistribute this output.',
                  'input_engine': str(engine_folder), 'input_game': str(game), 'input_modern_xg': str(modern),
                  'toolchain': {'vc': str(vc), 'sdk': str(sdk), 'sdk_version': version},
                  'generated_includes': includes, 'verification': verification, 'background': artwork,
                  'files': [{'path': p.relative_to(runtime).as_posix(), 'bytes': p.stat().st_size, 'sha256': digest(p)}
                            for p in sorted(runtime.rglob('*')) if p.is_file()]}
        (runtime/'preparation-report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
        (runtime/'pc/launcher-capabilities.json').write_text(json.dumps({
            'ready': True, 'native_pre_game': True, 'validated_resolutions': [1080,1440,2160,2880,3240,3780,4320],
            'validated_presets': ['low','default','high','ultra'], 'preset_status': {},
            'validation_scope': 'Pinned input/guard checks, native compilation, host/bridge contract and adapter DLL load. No game launched by preparation.'}, indent=2)+'\n', encoding='utf-8')
        os.rename(runtime, output)
        return {'ok': True, 'runtime_output': str(output), 'game_executed': False,
                'host_executed': False, 'files': len(report['files'])}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--engine-folder', type=Path, required=True)
    parser.add_argument('--game-folder', type=Path, required=True)
    parser.add_argument('--modern-xg', type=Path, required=True)
    parser.add_argument('--output', type=Path, default=ROOT/'runtime')
    parser.add_argument('--vc-dir', type=Path)
    parser.add_argument('--sdk-dir', type=Path)
    parser.add_argument('--sdk-version')
    parser.add_argument('--shader-compiler', type=Path)
    parser.add_argument('--background', type=Path, help='Optional user PNG instead of the included pre-game background.')
    args = parser.parse_args()
    try:
        result = prepare(args.engine_folder, args.game_folder, args.modern_xg, args.output,
                         vc_dir=args.vc_dir, sdk_dir=args.sdk_dir, sdk_version=args.sdk_version, shader_compiler=args.shader_compiler, background=args.background)
        print(json.dumps(result, indent=2))
        return 0
    except Exception as error:
        print(json.dumps({'ok': False, 'error': str(error)}), file=sys.stderr)
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
