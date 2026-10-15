"""Audit the exact Git source snapshot. Does not inspect or package local inputs."""
from pathlib import Path
import argparse
import hashlib
import json
import stat
import subprocess

ROOT = Path(__file__).resolve().parents[1]
TEXT_SUFFIXES = {'.c', '.cpp', '.h', '.asm', '.inc', '.py', '.cmd', '.def',
                 '.txt', '.md', '.json', '.cs', '.xaml', '.csproj', '.ps1'}
ASSETS = {
    'launcher/ForzaMastro.Launcher/Assets/Builder.png',
    'launcher/ForzaMastro.Launcher/Assets/Builder.ico',
    'launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport5.png',
    'launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport5.ico',
    'docs/screenshots/builder.png',
    'docs/screenshots/video.png',
}
PRIVATE_INCLUDES = {
    'fm5_language_guards.inc', 'fm5_resolve_original_guard.inc',
    'fm5_resolve_four_samples.inc', 'fm5_resolve_eight_samples.inc',
}
ALLOWED_ROOTS = {'src', 'tools', 'launcher', 'recipes', 'docs'}


def audit():
    result = subprocess.run(['git', 'ls-files', '--cached', '-z'], cwd=ROOT,
                            check=True, capture_output=True)
    names = [n.decode('utf-8') for n in result.stdout.split(b'\0') if n]
    if not names or len({n.casefold() for n in names}) != len(names):
        raise ValueError('Empty snapshot or case-insensitive path collision.')
    files = []
    for name in names:
        relative = Path(name)
        if relative.is_absolute() or '..' in relative.parts:
            raise ValueError('Source path escapes repository.')
        if len(relative.parts) > 1 and relative.parts[0] not in ALLOWED_ROOTS:
            raise ValueError('Unexpected source root: ' + name)
        if relative.name in PRIVATE_INCLUDES:
            raise ValueError('Private generated include entered Git: ' + name)
        path = ROOT / relative
        info = path.lstat()
        if not stat.S_ISREG(info.st_mode) or getattr(info, 'st_file_attributes', 0) & 0x400:
            raise ValueError('Source snapshot contains a nonordinary file: ' + name)
        raw = path.read_bytes()
        asset = name in ASSETS
        if not asset:
            if relative.suffix not in TEXT_SUFFIXES and relative.name != 'LICENSE' and name not in {'.gitignore', '.gitattributes'}:
                raise ValueError('Unexpected source/binary format: ' + name)
            raw.decode('utf-8-sig')
            if raw.startswith((b'MZ', b'DXBC', b'PK\x03\x04')):
                raise ValueError('Binary payload disguised as source: ' + name)
        files.append({'path': name, 'bytes': len(raw), 'sha256': hashlib.sha256(raw).hexdigest(),
                      'origin': 'Authored builder/icon asset' if asset else 'Authored/adapted project source, test, metadata or documentation'})
    return {'scope': 'FM5-only authored source snapshot; not a binary/game distribution',
            'passed': True, 'files_verified': len(files),
            'bytes': sum(f['bytes'] for f in files), 'files': files,
            'native_runtime_included': False, 'game_or_sdk_files_included': False,
            'native_runtime_build_complete': False}


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    report = audit()
    args.report.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in report.items() if key != 'files'}, indent=2))
