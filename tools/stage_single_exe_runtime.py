"""Stage an explicit, game-free component tree for the single-file Hub build.

Developer packaging only. This never starts a program, publishes an artifact,
copies a game directory, or replaces an existing component tree.
"""
from __future__ import annotations

import argparse
import configparser
import hashlib
import io
import json
from pathlib import Path
import shutil
import stat


RUNTIME_FILES = (
    'pc/ForzaMastro5.exe',
    'pc/data/licenses.txt',
    'pc/data/bin/avcodec-62.dll',
    'pc/data/bin/avutil-60.dll',
    'pc/data/bin/azimuth-tls.dll',
    'pc/data/bin/msvcp110.dll',
    'pc/data/bin/msvcr110.dll',
    'pc/data/ui/fm5-background-clean.png',
    'xg-host/kernelx.dll',
)


def sha256(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest()


def ordinary(path):
    path = Path(path).absolute()
    for item in (path, *path.parents):
        info = item.lstat()
        if stat.S_ISLNK(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('Packaging inputs must not traverse links: ' + str(path))
    if not path.is_file():
        raise ValueError('Required packaging input is missing: ' + str(path))
    return path


def copy_file(source, target):
    ordinary(source)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copyfile(source, target)


def stage(source_root, prepared_runtime, backend, bridge_template, bridge_manifest, destination):
    source_root, prepared_runtime, destination = map(Path, (source_root, prepared_runtime, destination))
    if not destination.is_absolute() or destination.exists():
        raise ValueError('Use a new absolute component staging directory.')
    ordinary(backend)
    ordinary(bridge_template)
    ordinary(bridge_manifest)
    from fm5_bridge_template import _load_manifest
    bridge_bytes = Path(bridge_template).read_bytes()
    _load_manifest(Path(bridge_manifest), bridge_bytes)
    if any(name in bridge_bytes for name in (
            b'fm5-host-xg.dll', 'fm5-host-xg.dll'.encode('utf-16-le'))):
        raise ValueError('The Bridge still requires the external modern XG library.')
    # All checks precede creation. In particular, never copy the original Bridge,
    # modern XG, either user SDK library, settings, saves, caches, logs or reports.
    for relative in RUNTIME_FILES:
        ordinary(prepared_runtime / relative)
    for relative in ('tools/pc_launcher_backend.py', 'launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport5.ico'):
        ordinary(source_root / relative)
    destination.mkdir(parents=True)
    for relative in RUNTIME_FILES:
        copy_file(prepared_runtime / relative, destination / 'runtime' / relative)
    copy_file(backend, destination / 'launcher/backend/ForzaMastro5Backend.exe')
    copy_file(bridge_template, destination / 'runtime/pc/ForzaMastro5Bridge.dll')
    copy_file(bridge_manifest, destination / 'runtime/bridge-template.json')
    for relative in ('tools/pc_launcher_backend.py', 'launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport5.ico'):
        copy_file(source_root / relative, destination / relative)
    ini = configparser.ConfigParser(interpolation=None)
    ini['launcher'] = dict(
        game='7b35d917', game_folder='', resolution='2160', aspect='16:9',
        antialiasing='msaax4', graphics='ultra', shadows='ultra', reflections='ultra',
        car_detail='ultra', particles='ultra', motion_blur='ultra', texture_filtering='ultra',
        audio='multichannel', frame_rate='120', frame_pacing='real-time', vsync='on',
        display='fullscreen', hdr='off', language='it-IT', speech='system', player_name='Player 1')
    stream = io.StringIO()
    ini.write(stream, space_around_delimiters=False)
    (destination / 'runtime/pc/data/RaceOneEngine.ini').write_text(stream.getvalue(), encoding='utf-8')
    capabilities = dict(ready=True, native_pre_game=True,
        validated_resolutions=[1080,1440,2160,2880,3240,3780,4320],
        validated_presets=['low','default','high','ultra'], preset_status={},
        bridge_template=True, user_game_required=True)
    (destination / 'runtime/pc/launcher-capabilities.json').write_text(json.dumps(capabilities, indent=2)+'\n', encoding='utf-8')
    notices = source_root / 'launcher/THIRD-PARTY-NOTICES.txt'
    copy_file(notices, destination / 'THIRD-PARTY-NOTICES.txt')
    copy_file(source_root / 'LICENSE', destination / 'LICENSE')
    forbidden = {'xg.dll', 'xg_x.dll', 'fm5-host-xg.dll', 'forza_durango_release_final.exe'}
    files = []
    for path in sorted(destination.rglob('*')):
        if not path.is_file():
            continue
        if path.name.casefold() in forbidden:
            raise ValueError('Game library entered the component payload: ' + str(path))
        files.append(dict(path=path.relative_to(destination).as_posix(), bytes=path.stat().st_size, sha256=sha256(path)))
    return dict(component_root=str(destination), files=files, user_game_included=False,
                modern_xg_included=False, bridge_is_unhydrated_template=True, game_executed=False)


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    for name in ('source-root', 'prepared-runtime', 'backend', 'bridge-template', 'bridge-manifest', 'destination'):
        parser.add_argument('--'+name, type=Path, required=True)
    args = parser.parse_args()
    print(json.dumps(stage(**vars(args)), indent=2))
