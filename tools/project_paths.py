"""Location helpers shared by build tools and private reverse-engineering scripts.

Library discovery never depends on the current directory or a sibling project.
Optional FH2 inputs are private research dependencies, supplied explicitly via
environment variables; normal FM5 builder operations do not require them.
"""
from pathlib import Path
import os
import sys


def project_root(start):
    """Find this source tree from a script/directory, including nested tools."""
    location = Path(start).resolve()
    if not location.is_dir():
        location = location.parent
    for candidate in (location, *location.parents):
        if (candidate / 'tools/project_paths.py').is_file() and (candidate / 'src').is_dir():
            return candidate
    raise RuntimeError(f'Race One Hub source root not found from {start}')


def add_python_libraries(start):
    """Use the local, optionally junction-backed PE tooling dependency folder."""
    libraries = project_root(start) / 'tools/python-libs'
    if not libraries.is_dir():
        return None  # A public checkout uses normal packages from its activated virtualenv.
    value = str(libraries)
    if value not in sys.path:
        sys.path.insert(0, value)
    return libraries


def project_path(value, root):
    """Resolve stored profile paths; relative paths are rooted in this repo."""
    path = Path(value)
    if not path.is_absolute():
        path = Path(root) / path
    return path.resolve()


def _external_directory(variable, description):
    value = os.environ.get(variable)
    if not value:
        raise RuntimeError(f'{description} is an optional private research input. Set {variable} to its directory.')
    path = Path(value).expanduser()
    if not path.is_absolute() or not path.is_dir():
        raise RuntimeError(f'{variable} must name an existing absolute directory: {value}')
    return path.resolve()


def external_fh2_root():
    """Private FH2 analysis workspace; never inferred from the FM5 location."""
    return _external_directory('RACEONE_FH2_REFERENCE', 'The FH2 analysis workspace')


def external_fh2_game():
    """Extracted FH2 package root containing Mount/ and EmbeddedXvd/."""
    return _external_directory('RACEONE_FH2_GAME', 'The extracted FH2 package')
