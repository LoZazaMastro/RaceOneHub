"""Validate explicit user-owned FM5/original engine inputs; never download or extract them."""
from pathlib import Path
import argparse
import hashlib
import json
from fm5_game_files import validate_game_source

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--game-folder', type=Path, required=True)
parser.add_argument('--azimuth-directory', type=Path, required=True)
args = parser.parse_args()
game = validate_game_source(args.game_folder)
engine = args.azimuth_directory / 'azimuth.exe'
expected = '579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc'
if not engine.is_file() or hashlib.sha256(engine.read_bytes()).hexdigest() != expected:
    parser.error('Supply the supported original 0.9.9 engine directory. No original is bundled.')
print(json.dumps({'validated': True, 'game': game, 'engine': str(engine.resolve()),
                  'files_copied': False, 'game_executed': False}, indent=2))
