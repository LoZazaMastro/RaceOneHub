"""Clean-package the backend and verify current embedded helper modules."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import subprocess
import sys
import types


def code_fingerprint(code):
    def value(item):
        if isinstance(item, types.CodeType):
            return [item.co_code.hex(), list(item.co_names), list(item.co_varnames),
                    item.co_argcount, item.co_kwonlyargcount, item.co_flags,
                    list(item.co_freevars), list(item.co_cellvars),
                    [value(entry) for entry in item.co_consts]]
        if isinstance(item, tuple):
            return [value(entry) for entry in item]
        return [type(item).__name__, repr(item)]
    return hashlib.sha256(json.dumps(value(code), sort_keys=True).encode()).hexdigest()


def inspect_embedded_helper(executable, source):
    from PyInstaller.archive.readers import CArchiveReader
    archive = CArchiveReader(str(executable)).open_embedded_archive('PYZ.pyz')
    embedded = archive.extract('pc_icon_resources')
    compiled = compile(source.read_bytes(), 'pc_icon_resources.py', 'exec')
    expected = code_fingerprint(compiled)
    actual = code_fingerprint(embedded)
    if actual != expected:
        raise RuntimeError('Packaged icon helper does not match the current source.')
    if 'pefile' not in archive.toc:
        raise RuntimeError('Packaged backend is missing pefile.')
    return {'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
            'source_code_fingerprint': expected, 'embedded_code_fingerprint': actual,
            'embedded_matches_current_source': True, 'pefile_present': True}


def inspect_embedded_modules(executable, sources):
    from PyInstaller.archive.readers import CArchiveReader
    archive = CArchiveReader(str(executable)).open_embedded_archive('PYZ.pyz')
    result = {}
    for name, source in sources.items():
        if name not in archive.toc:
            raise RuntimeError('Required runtime helper is missing from the frozen backend: ' + name)
        actual = code_fingerprint(archive.extract(name))
        expected = code_fingerprint(compile(source.read_bytes(), name+'.py', 'exec'))
        if actual != expected:
            raise RuntimeError('Frozen helper does not match current source: ' + name)
        result[name] = {'source_sha256': hashlib.sha256(source.read_bytes()).hexdigest(),
                        'code_fingerprint': actual, 'embedded_matches_current_source': True}
    return result


def main():
    root = Path(__file__).resolve().parents[1]
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output-dir', type=Path, default=root/'launcher/backend')
    parser.add_argument('--work-dir', type=Path, default=root/'analysis/launcher-packaging')
    args = parser.parse_args()
    work = args.work_dir.resolve()
    work.mkdir(parents=True, exist_ok=True)
    environment = os.environ.copy()
    environment['PYTHONPATH'] = os.pathsep.join(str(root/p) for p in
        ['tools/packaging-libs', 'tools/python-libs', 'tools'])
    # A private cache prefix prevents a same-timestamp stale source pyc from
    # entering the bundle. --clean also removes PyInstaller analysis caches.
    environment['PYTHONPYCACHEPREFIX'] = str(work/'fresh-python-cache')
    command = [sys.executable, '-B', '-m', 'PyInstaller', '--clean', '--noconfirm', '--onefile',
        '--name', 'ForzaMastro5Backend', '--distpath', str(args.output_dir.resolve()),
        '--workpath', str(work), '--specpath', str(work),
        '--paths', str(root/'tools'), '--paths', str(root/'tools/python-libs'),
        '--hidden-import', 'pefile', '--hidden-import', 'fm5_bridge_template',
        '--hidden-import', 'generate_fm5_includes',
        '--add-data', str(root/'tools/fm5_create_shortcut.ps1') + os.pathsep + '.',
        str(root/'tools/pc_launcher_backend.py')]
    subprocess.run(command, cwd=root, env=environment, check=True)
    sys.path.insert(0, str(root/'tools/packaging-libs'))
    executable = args.output_dir.resolve()/'ForzaMastro5Backend.exe'
    report = inspect_embedded_helper(executable, root/'tools/pc_icon_resources.py')
    report['runtime_helpers'] = inspect_embedded_modules(executable, {
        name:root/'tools'/(name+'.py') for name in ['fm5_bridge_template','generate_fm5_includes']})
    report.update(executable=str(executable), executable_sha256=hashlib.sha256(executable.read_bytes()).hexdigest())
    (work/'embedded-source-verification.json').write_text(json.dumps(report, indent=2)+'\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
