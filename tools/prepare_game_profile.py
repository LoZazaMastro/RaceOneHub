from pathlib import Path
import argparse
import hashlib
import json
import shutil
import time
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser()
parser.add_argument('--all-scenes', action='store_true')
arguments = parser.parse_args()
SOURCE = ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe'
TARGET = ROOT / 'runtime/game-60fps'
REPORT = ROOT / 'runtime/game-60fps-profile.json'
SCENARIOS = ['FrontEnd.xml', 'MiniAutoVista.xml', 'PreRace_321.xml', 'PostRace.xml']
RELATIVE = Path('Mount/media/renderscenarios')
if arguments.all_scenes:
    SCENARIOS = [p.name for p in (SOURCE / RELATIVE).glob('*.xml')
                 if (node := ET.parse(p).getroot().find('FrameRate')) is not None
                 and node.attrib['value'] == '30']
originals = {name: (SOURCE / RELATIVE / name).read_bytes() for name in SCENARIOS}
old = b'<FrameRate value="30"/>'
new = b'<FrameRate value="60"/>'
for name, data in originals.items():
    assert data.count(old) == 1, name
    assert ET.fromstring(data).find('FrameRate').attrib['value'] == '30', name

files = [p for p in SOURCE.rglob('*') if p.is_file()]
total = sum(p.stat().st_size for p in files)
if shutil.disk_usage(ROOT).free < total and not TARGET.exists():
    raise OSError('Not enough free space for the independent game profile')
REPORT.parent.mkdir(parents=True, exist_ok=True)
REPORT.write_text(json.dumps({'ready': False, 'source': str(SOURCE),
                              'target': str(TARGET)}, indent=2) + '\n')
copied = 0
reused = 0
last_progress = time.monotonic()

def copy_file(source, target):
    global copied, reused, last_progress
    source, target = Path(source), Path(target)
    a = source.stat()
    if target.is_file():
        b = target.stat()
        if a.st_size == b.st_size and a.st_mtime_ns == b.st_mtime_ns:
            reused += 1
            return str(target)
    shutil.copy2(source, target)
    copied += 1
    if time.monotonic() - last_progress >= 10:
        print(f'Copied {copied} files; reused {reused}', flush=True)
        last_progress = time.monotonic()
    return str(target)

shutil.copytree(SOURCE, TARGET, dirs_exist_ok=True, copy_function=copy_file)
changes = []
for name, data in originals.items():
    path = TARGET / RELATIVE / name
    assert path.read_bytes() == data, name
    modified = data.replace(old, new)
    assert ET.fromstring(modified).find('FrameRate').attrib['value'] == '60'
    path.write_bytes(modified)
    assert (SOURCE / RELATIVE / name).read_bytes() == data
    changes.append({'path': str(RELATIVE / name),
                    'before_sha256': hashlib.sha256(data).hexdigest(),
                    'after_sha256': hashlib.sha256(modified).hexdigest(),
                    'change': 'FrameRate 30 -> 60; all other bytes preserved'})

report = {'ready': True, 'source': str(SOURCE), 'target': str(TARGET),
          'files': len(files), 'source_bytes': total,
          'copied_files': copied, 'reused_files': reused,
          'changes': changes, 'hardlinks_used': False,
          'all_scenes': arguments.all_scenes,
          'original_files_modified': False,
          'scope': 'Independent game files; selected scenario FrameRate settings use the native 60 mode. Desktop presentation target is configured separately in Azimuth. Visual/performance verification pending.'}
REPORT.write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2), flush=True)
