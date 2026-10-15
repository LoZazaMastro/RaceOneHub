"""Local PC builder protocol and internal diagnostic launch helper."""
from pathlib import Path
import argparse
import configparser
import contextlib
import ctypes as ct
from ctypes import wintypes as wt
import datetime
import io
import json
import os
import shutil
import subprocess
import sys
import tempfile
import time
from fm5_language_catalog import TEXT_LANGUAGES, text_labels
from raceone_games import GAMES, game_definition
from fm5_game_files import validate_game_source, stage_scene_overrides, create_desktop_shortcut
from pc_icon_resources import embed_icon_in_place


def find_root():
    start = Path(sys.executable if getattr(sys, 'frozen', False) else __file__).resolve()
    for candidate in start.parents:
        if (candidate / 'launcher').is_dir() and (candidate / 'tools/pc_launcher_backend.py').is_file():
            return candidate
    raise RuntimeError('Game folder not found next to the builder.')


SUPPORTED = {
    'resolution': [1080, 1440, 2160, 2880, 3240, 3780, 4320], 'frame_rate': [30, 60, 120],
    'antialiasing': ['off', 'msaax2', 'msaax4'],
    'graphics': ['low', 'default', 'high', 'ultra'],
    'display': ['fullscreen', 'windowed'], 'vsync': [True, False],
    'language': list(TEXT_LANGUAGES), 'audio': ['stereo', 'multichannel'],
    'show_pre_game': [True, False],
}
DEFAULTS = dict(resolution=2160, frame_rate=120, antialiasing='msaax4',
                graphics='ultra', display='fullscreen', vsync=True,
                language='it-IT', audio='multichannel', show_pre_game=True)
GROUPS = ['shadows', 'reflections', 'car_detail', 'particles',
          'motion_blur', 'texture_filtering']


def running_pids(root, game_id="fm5"):
    game = game_definition(game_id)
    """Match the full executable path; do not touch other games or processes."""
    class ProcessEntry(ct.Structure):
        _fields_ = [('size', wt.DWORD), ('usage', wt.DWORD), ('pid', wt.DWORD),
                    ('heap', ct.c_size_t), ('module', wt.DWORD), ('threads', wt.DWORD),
                    ('parent', wt.DWORD), ('priority', wt.LONG), ('flags', wt.DWORD),
                    ('exe', wt.WCHAR * 260)]
    k = ct.WinDLL('kernel32', use_last_error=True)
    k.CreateToolhelp32Snapshot.argtypes = [wt.DWORD, wt.DWORD]
    k.CreateToolhelp32Snapshot.restype = wt.HANDLE
    k.Process32FirstW.argtypes = k.Process32NextW.argtypes = [wt.HANDLE, ct.POINTER(ProcessEntry)]
    k.OpenProcess.argtypes = [wt.DWORD, wt.BOOL, wt.DWORD]
    k.OpenProcess.restype = wt.HANDLE
    k.QueryFullProcessImageNameW.argtypes = [wt.HANDLE, wt.DWORD, wt.LPWSTR, ct.POINTER(wt.DWORD)]
    k.CloseHandle.argtypes = [wt.HANDLE]
    snapshot = k.CreateToolhelp32Snapshot(2, 0)
    if snapshot == ct.c_void_p(-1).value:
        raise ct.WinError(ct.get_last_error())
    target = os.path.normcase(str((root / game.runtime / 'ForzaMastro5.exe').resolve()))
    found = []
    try:
        item = ProcessEntry(); item.size = ct.sizeof(item)
        more = k.Process32FirstW(snapshot, ct.byref(item))
        while more:
            if item.exe.lower() == 'forzamastro5.exe':
                process = k.OpenProcess(0x1000, False, item.pid)
                if not process:
                    # Conservatively refuse an update if a namesake cannot be inspected.
                    raise RuntimeError('Cannot inspect a ForzaMastro5 process. Close the game before continuing.')
                try:
                    length = wt.DWORD(32768); name = ct.create_unicode_buffer(length.value)
                    if not k.QueryFullProcessImageNameW(process, 0, name, ct.byref(length)):
                        raise ct.WinError(ct.get_last_error())
                    if os.path.normcase(name.value) == target:
                        found.append(item.pid)
                finally:
                    k.CloseHandle(process)
            more = k.Process32NextW(snapshot, ct.byref(item))
    finally:
        k.CloseHandle(snapshot)
    return found


def load_ini(root, game_id="fm5"):
    game = game_definition(game_id)
    result = configparser.ConfigParser(interpolation=None)
    result.read(root / game.runtime / 'data/RaceOneEngine.ini', encoding='utf-8-sig')
    return result


def capabilities(root, game_folder=None, game_id="fm5"):
    game = game_definition(game_id)
    current = DEFAULTS.copy()
    ini = load_ini(root, game_id)
    if ini.has_section('launcher'):
        section = ini['launcher']
        for key in current:
            value = section.get(key, str(current[key]))
            try:
                current[key] = (value.lower() in ['on', 'true', '1'] if key in ['vsync', 'show_pre_game'] else
                                int(value) if key in ['resolution', 'frame_rate'] else value)
            except ValueError:
                pass
    if current['antialiasing'] == 'msaax8':
        current['antialiasing'] = 'msaax4'  # Retired visible choice; preserve legacy preferences safely.
    manifest_path = root / game.runtime / 'launcher-capabilities.json'
    manifest = json.loads(manifest_path.read_text('utf-8')) if manifest_path.is_file() else {}
    ready = bool(manifest.get('ready')) and all((root / game.runtime / p).is_file()
        for p in ['ForzaMastro5.exe', 'ForzaMastro5Bridge.dll', 'data/RaceOneEngine.ini'])
    selected_game = str(game_folder or ini.get('launcher', 'game_folder', fallback=''))
    if Path(selected_game).name.casefold() == 'mount':
        selected_game = str(Path(selected_game).parent)
    labels = text_labels(selected_game)
    supported = dict(SUPPORTED, language=list(labels))
    if current['language'] not in labels and labels:
        current['language'] = 'it-IT' if 'it-IT' in labels else next(iter(labels))
    return {'schema_version': 1, 'game_id': game_id, 'ready': ready,
            'build_ready': ready and bool(manifest.get('native_pre_game')),
            'output_filename': game.executable,
            'running': bool(running_pids(root, game_id)), 'current': current, 'supported': supported,
            'language_labels': labels,
            'game_folder': selected_game, 'copies_user_game_files': False,
            'desktop_shortcut_supported': True,
            'preset_status': manifest.get('preset_status', {}),
            'warnings': ['120 FPS is experimental.'] +
                        ([] if ready else ['Supply and prepare the required runtime inputs before building. See docs/BUILDING.md.'])}


def validate_settings(settings):
    if isinstance(settings, dict):
        settings = dict(settings)
        if settings.get('antialiasing') == 'msaax8':
            settings['antialiasing'] = 'msaax4'
        settings.pop('speech', None)  # Legacy independent voice setting is retired.
        if settings.get('language') == 'system':
            settings['language'] = DEFAULTS['language']
    if isinstance(settings, dict) and set(settings) <= set(SUPPORTED):
        # Migrate profiles created before the pre-game setting existed.
        missing = set(SUPPORTED) - set(settings)
        if missing <= {'show_pre_game'}:
            settings = dict(settings, **{k: DEFAULTS[k] for k in missing})
    if not isinstance(settings, dict) or set(settings) != set(SUPPORTED):
        raise ValueError('The profile must contain exactly the supported settings.')
    for key, choices in SUPPORTED.items():
        if type(settings[key]) is not type(choices[0]) or settings[key] not in choices:
            raise ValueError(f'Unsupported value for {key}.')
    return dict(settings)


def atomic_write(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    fd, temp_name = tempfile.mkstemp(prefix=path.name + '.', suffix='.tmp', dir=path.parent)
    try:
        options = {} if isinstance(content, bytes) else {'encoding': 'utf-8', 'newline': '\n'}
        with os.fdopen(fd, 'wb' if isinstance(content, bytes) else 'w', **options) as stream:
            stream.write(content); stream.flush(); os.fsync(stream.fileno())
        os.replace(temp_name, path)
    finally:
        if os.path.exists(temp_name):
            os.unlink(temp_name)


def configured_ini(root, settings, game_folder=None, game_id="fm5"):
    game = game_definition(game_id)
    ini = load_ini(root, game_id)
    if not ini.has_section("launcher"):
        ini.add_section("launcher")
    section = ini['launcher']
    for key, value in settings.items():
        if key == 'show_pre_game':
            continue
        section[key] = ('on' if value else 'off') if key == 'vsync' else str(value)
    for group in GROUPS:
        section[group] = settings['graphics']
    section['frame_pacing'] = 'real-time'; section['hdr'] = 'off'
    section['speech'] = 'system'  # Native meaning: speech follows the selected game language.
    if game_folder is not None:
        section['game_folder'] = str(game_folder)
        section['game'] = game.title_id
        title = 'title-' + game.title_id
        if not ini.has_section(title):
            ini.add_section(title)
        ini[title]['folder'] = str(game_folder)
        ini[title]['name'] = game.name
    stream = io.StringIO(); ini.write(stream, space_around_delimiters=False)
    return stream.getvalue()


def build_game(root, settings, output, game_folder=None, desktop_shortcut=False, progress_file=None, game_id="fm5"):
    game = game_definition(game_id)
    """Create a fresh bundle without modifying the active runtime or user files."""
    settings = validate_settings(settings)
    if type(desktop_shortcut) is not bool:
        raise ValueError('Desktop shortcut must be enabled or disabled.')
    output = Path(output)
    if not output.is_absolute() or output.name != game.executable:
        raise ValueError(f'Choose an absolute output path named {game.executable}.')
    output = output.resolve()
    destination = output.parent
    with launch_lock(root):
        if game_folder is None:
            raise ValueError('Choose the folder containing your Forza Motorsport 5 game files.')
        source = validate_game_source(game_folder, game_id)
        game_folder = Path(source['game_root'])
        state = capabilities(root, game_folder, game_id)
        if state['running']:
            raise RuntimeError('Close ForzaMastro5 before building.')
        if not state['build_ready']:
            raise RuntimeError('The updated builder runtime is not ready yet.')
        manifest = json.loads((root / game.runtime / 'launcher-capabilities.json').read_text('utf-8'))
        if settings['resolution'] not in manifest.get('validated_resolutions', []):
            raise RuntimeError('This resolution has not passed profile validation yet.')
        if settings['graphics'] not in manifest.get('validated_presets', []):
            raise RuntimeError('This preset has not passed profile validation yet.')
        if settings['language'] not in state['supported']['language']:
            raise RuntimeError('The selected language is not present in your game files.')
        if destination == root.resolve() or root.resolve() in destination.parents and any(
                area.resolve() == destination or area.resolve() in destination.parents
                for area in [root/'runtime', root/'game', root/'src', root/'tools', root/'launcher']):
            raise ValueError('Choose a separate output folder outside the project components.')
        if destination.exists() and (not destination.is_dir() or any(destination.iterdir())):
            raise ValueError('Choose an empty output folder. Existing files will not be replaced.')
        if destination == game_folder or game_folder in destination.parents or destination in game_folder.parents:
            raise ValueError('Choose an output folder separate from your game source folder.')
        destination.parent.mkdir(parents=True, exist_ok=True)
        stage = Path(tempfile.mkdtemp(prefix='.forzamastro-build-', dir=destination.parent))
        removed_empty = False
        last_progress_time = 0.0
        last_progress_phase = None

        def progress(event):
            nonlocal last_progress_time, last_progress_phase
            now = time.monotonic()
            if progress_file is not None and (event['phase'] != last_progress_phase or now - last_progress_time >= 0.1):
                atomic_write(Path(progress_file), json.dumps(event) + '\n')
                last_progress_time, last_progress_phase = now, event['phase']

        try:
            game_reference = stage_scene_overrides(game_folder, stage, settings['frame_rate'], progress=progress, game_id=game_id)
            progress(dict(phase='creating_executable', bytes_copied=game_reference['override_bytes'],
                          total_bytes=game_reference['override_bytes'], files_copied=0, total_files=0))
            runtime = stage
            for name in ['ForzaMastro5.exe', 'ForzaMastro5Bridge.dll']:
                shutil.copy2(root / game.runtime / name,
                             runtime / (output.name if name.endswith('.exe') else name))
            icon = root / ('launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport' +
                           '5' + '.ico')
            embed_icon_in_place(runtime / output.name, icon)
            (runtime / 'data/bin').mkdir(parents=True)
            for dependency in ['avcodec-62.dll', 'avutil-60.dll', 'azimuth-tls.dll', 'msvcp110.dll', 'msvcr110.dll']:
                shutil.copy2(root / game.runtime / 'data/bin' / dependency, runtime / 'data/bin' / dependency)
            shutil.copy2(root / game.runtime / 'data/licenses.txt', runtime / 'data/licenses.txt')
            atomic_write(runtime / 'data/RaceOneEngine.ini', configured_ini(root, settings, game_folder, game_id))
            atomic_write(runtime / 'data/forzamastro-languages.json',
                         json.dumps(state['language_labels'], indent=2) + '\n')
            atomic_write(stage / 'forzamastro.ini',
                         '[startup]\nshow_pre_game=' + ('1' if settings['show_pre_game'] else '0') + '\n')
            shutil.copytree(root / 'runtime/xg-host', stage / 'xg-host',
                            ignore=shutil.ignore_patterns('*.obj', '*.lib', '*.exp'))
            for name in game.sdk_libraries:
                shutil.copy2(game_folder / 'EmbeddedXvd/Windows/System32' / name, stage / 'xg-host' / name)
            assets = root / 'pregame/assets'
            background = assets / 'fm5-background-clean.png'
            if background.is_file():
                (stage / 'data/ui').mkdir(parents=True)
                shutil.copy2(background, stage / 'data/ui' / background.name)
            atomic_write(stage / 'build-profile.json', json.dumps({
                'game': game.name, 'game_id': game_id, 'executable': output.name,
                'settings': settings, 'game_source': str(game_folder),
                'game_folder': str(game_folder),
                'game_files_copied': False, 'game_reference': game_reference,
                'custom_pre_game_screen': settings['show_pre_game'],
                'pre_game_process': 'same as game',
                'game_derived_sdk_libraries_copied': list(game.sdk_libraries),
                'private_local_test_bundle': True,
                'public_distribution': False,
                'scope': 'Local Windows PC build using files supplied by the user.'
            }, indent=2) + '\n')
            atomic_write(stage / 'README.txt',
                f'Run {game.executable}. Keep all generated files together.\n'
                'The game is read from your selected original folder. Keep that folder available at the same path.\n'
                'Private local test bundle: includes SDK libraries from your game files. Do not publish this bundle as the builder release.\n'
                'Pre-game settings: ' + ('enabled' if settings['show_pre_game'] else 'disabled') + '.\n'
                'Change data/RaceOneEngine.ini manually or build into a new folder to change settings.\n'
                '120 FPS is experimental.\n' +
                ('Known issue: colors can look washed out in Career after a race. Press B to return to mode selection, then enter Career again.\n'
                 if game_id == 'fm5' else ''))
            if destination.exists():
                destination.rmdir()  # Non-recursive: refuses any concurrent new file.
                removed_empty = True
            os.rename(stage, destination)
        except Exception:
            if removed_empty and not destination.exists():
                destination.mkdir()
            raise
        finally:
            if stage.exists():
                if stage.resolve().parent != destination.parent.resolve() or not stage.name.startswith('.forzamastro-build-'):
                    raise RuntimeError('Refusing cleanup outside this build staging directory.')
                def writable_remove(function, path, error_info):
                    os.chmod(path, 0o700)
                    function(path)
                shutil.rmtree(stage, onerror=writable_remove)
        shortcut = None
        warnings = []
        if desktop_shortcut:
            try:
                shortcut = create_desktop_shortcut(output, package_committed=True, **({'name': Path(game.executable).stem} if game_id != 'fm5' else {}))
            except Exception as error:
                warnings.append('Game built successfully; desktop shortcut was not created: ' + str(error))
        return {'ok': True, 'output': str(output), 'bundle_directory': str(destination),
                'game_files_copied': False, 'settings': settings,
                'desktop_shortcut': shortcut, 'warnings': warnings}


@contextlib.contextmanager
def launch_lock(root):
    import hashlib
    k = ct.WinDLL('kernel32', use_last_error=True)
    k.CreateMutexW.argtypes = [ct.c_void_p, wt.BOOL, wt.LPCWSTR]
    k.CreateMutexW.restype = wt.HANDLE
    k.ReleaseMutex.argtypes = k.CloseHandle.argtypes = [wt.HANDLE]
    name = 'Local\\ForzaMastro5Launch-' + hashlib.sha256(str(root).encode()).hexdigest()[:20]
    handle = k.CreateMutexW(None, True, name)
    if not handle:
        raise ct.WinError(ct.get_last_error())
    owned = ct.get_last_error() != 183
    try:
        if not owned:
            raise RuntimeError('Another launch is already in progress.')
        yield
    finally:
        if owned:
            k.ReleaseMutex(handle)
        k.CloseHandle(handle)


def apply_and_launch(root, settings, launch):
    settings = validate_settings(settings)
    with launch_lock(root):
        state = capabilities(root)
        if state['running']:
            raise RuntimeError('Close ForzaMastro5 before changing settings or starting it again.')
        if not state['ready']:
            raise RuntimeError('The updated runtime is not ready yet.')
        manifest = json.loads((root / 'runtime/pc/launcher-capabilities.json').read_text('utf-8'))
        if settings['resolution'] not in manifest.get('validated_resolutions', []):
            raise RuntimeError('This resolution has not passed profile validation yet.')
        if settings['graphics'] not in manifest.get('validated_presets', []):
            raise RuntimeError('This preset has not passed profile validation yet.')
        path = root / 'runtime/pc/data/RaceOneEngine.ini'
        old_ini = path.read_bytes()
        atomic_write(path, configured_ini(root, settings))
        if not launch:
            return {'ok': True, 'settings': settings}
        stamp = datetime.datetime.now().strftime('%Y%m%d-%H%M%S-%f')
        logs = root / 'analysis/pc-build'; logs.mkdir(parents=True, exist_ok=True)
        stdout = logs / f'{stamp}-launcher-stdout.log'
        stderr = logs / f'{stamp}-launcher-stderr.log'
        environment = os.environ.copy()
        for key in list(environment):
            if key.startswith('FM5_'):
                del environment[key]
        environment.update(manifest.get('diagnostic_environment', {}))
        command = [str(root / 'runtime/pc/ForzaMastro5.exe'), '--skip-launcher']
        try:
            with stdout.open('wb') as out, stderr.open('wb') as err:
                process = subprocess.Popen(command, cwd=root / 'runtime/pc', env=environment,
                    stdin=subprocess.DEVNULL, stdout=out, stderr=err, close_fds=True,
                    creationflags=subprocess.DETACHED_PROCESS | subprocess.CREATE_NEW_PROCESS_GROUP)
        except OSError:
            atomic_write(path, old_ini)
            raise
        session = logs / 'interactive-session.json'
        atomic_write(session, json.dumps({'pid': process.pid, 'command': command,
            'started_local': stamp, 'stdout': str(stdout), 'stderr': str(stderr),
            'automatic_stop': False, 'settings': settings,
            'diagnostic_child_environment': manifest.get('diagnostic_environment', {})}, indent=2) + '\n')
        return {'ok': True, 'pid': process.pid, 'session_file': str(session)}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('--game', choices=list(GAMES), default='fm5')
    parser.add_argument('--capabilities', action='store_true')
    parser.add_argument('--settings', type=Path)
    parser.add_argument('--launch', action='store_true')
    parser.add_argument('--build', action='store_true')
    parser.add_argument('--output', type=Path)
    parser.add_argument('--game-folder', type=Path)
    parser.add_argument('--desktop-shortcut', action='store_true')
    parser.add_argument('--progress-file', type=Path)
    args = parser.parse_args()
    try:
        root = find_root()
        if args.capabilities and not args.settings and not args.launch and not args.build and not args.output:
            result = capabilities(root, args.game_folder, args.game)
        elif args.settings and args.build and args.output and not args.launch and not args.capabilities:
            result = build_game(root, json.loads(args.settings.read_text('utf-8-sig')), args.output,
                                args.game_folder, args.desktop_shortcut, args.progress_file, args.game)
        elif args.settings and not args.capabilities and not args.build and not args.output:
            if args.game != 'fm5':
                raise ValueError('Direct diagnostic launch is not available for this game yet.')
            result = apply_and_launch(root, json.loads(args.settings.read_text('utf-8-sig')), args.launch)
        else:
            raise ValueError('Use --capabilities or --settings FILE --build --output EXE.')
        print(json.dumps(result, ensure_ascii=True))
        return 0
    except Exception as error:
        print(json.dumps({'ok': False, 'error': str(error)}, ensure_ascii=True))
        return 1


if __name__ == '__main__':
    raise SystemExit(main())
