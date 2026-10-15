"""Exercise the actual native file wrapper on synthetic files, never the game."""
from pathlib import Path
import ctypes as C
import contextlib
from ctypes import wintypes as W
import json
import shutil
import struct
import sys
import tempfile
import os
from project_paths import add_python_libraries

ROOT = Path(__file__).resolve().parents[1]
add_python_libraries(__file__)
import pefile

checks = 0
def check(value, message):
    global checks
    assert value, message
    checks += 1

with tempfile.TemporaryDirectory(prefix='raceone-scene-files-') as temp, contextlib.ExitStack() as cleanup:
    base = Path(temp)
    original = base / ('original ' + chr(233))
    scene = original / 'Mount/media/renderscenarios'
    scene.mkdir(parents=True)
    local = base / 'app'
    (local / 'data/scene-overrides').mkdir(parents=True)
    names = ['FrontEnd.xml', 'MiniAutoVista.xml', 'PhotoMode.xml', 'PostRace.xml', 'PreRace_321.xml', 'Replay.xml']
    for name in names:
        (scene / name).write_bytes(b'original 30: ' + name.encode())
        (local / 'data/scene-overrides' / name).write_bytes(b'override 60: ' + name.encode())
    (scene / 'Race.xml').write_bytes(b'unchanged race')
    unrelated = base / 'other/FrontEnd.xml'
    unrelated.parent.mkdir(); unrelated.write_bytes(b'other title')
    (local / 'data/RaceOneEngine.ini').write_text('[launcher]\ngame_folder=' + str(original) + '\n', encoding='utf-8')
    dll_path = local / 'scene-file-fixture.dll'
    shutil.copy2(ROOT / 'runtime/pc/ForzaMastro5Bridge.dll', dll_path)
    dll = C.WinDLL(str(dll_path), use_last_error=True)
    k = C.WinDLL('kernel32', use_last_error=True)
    k.FreeLibrary.argtypes = [W.HMODULE]
    cleanup.callback(k.FreeLibrary, dll._handle)
    dll.FM5SetSceneFileRate.argtypes = [W.UINT]
    dll.FM5SetSceneFileRate.restype = None
    dll.FM5CreateGameFileW.argtypes = [W.LPCWSTR, W.DWORD, W.DWORD, C.c_void_p, W.DWORD, W.DWORD, W.HANDLE]
    dll.FM5CreateGameFileW.restype = W.HANDLE
    k.ReadFile.argtypes = [W.HANDLE, C.c_void_p, W.DWORD, C.POINTER(W.DWORD), C.c_void_p]
    k.CloseHandle.argtypes = [W.HANDLE]
    invalid = C.c_void_p(-1).value
    def read(path, access=0x80000000, flags=0x80):
        handle = dll.FM5CreateGameFileW(str(path), access, 3, None, 3, flags, None)
        if handle == invalid:
            return None, C.get_last_error()
        try:
            data = C.create_string_buffer(1024); count = W.DWORD()
            check(k.ReadFile(handle, data, 1024, C.byref(count), None), 'native ReadFile')
            return data.raw[:count.value], None
        finally: k.CloseHandle(handle)
    original_bytes = {p.name:p.read_bytes() for p in scene.iterdir()}
    for fps in (30, 60, 120, 30):
        dll.FM5SetSceneFileRate(fps)
        for name in names:
            expected = ('original 30: ' if fps == 30 else 'override 60: ').encode()+name.encode()
            check(read(scene / name)[0] == expected, (fps, name))
        check(read(scene / 'Race.xml')[0] == b'unchanged race', 'unrelated scene passes through')
        check(read(unrelated)[0] == b'other title', 'same basename outside selected root passes through')
    dll.FM5SetSceneFileRate(120)
    source = scene / names[0]
    for encoding in ('utf-8', 'utf-8-sig', 'utf-16'):
        (local / 'data/RaceOneEngine.ini').write_text('[other]\nvalue=unrelated\n\n[launcher]\ngame_folder=' + str(original) + '\n', encoding=encoding)
        check(read(source)[0] == b'override 60: FrontEnd.xml', ('INI path encoding', encoding))
    for path in (str(source).upper(), str(source).replace('\\', '/'), str(scene / '..' / 'renderscenarios' / names[0]), '\\\\?\\' + str(source)):
        check(read(path)[0] == b'override 60: FrontEnd.xml', ('canonical equivalent source', path, read(path)))
    before_cwd = Path.cwd()
    try:
        os.chdir(original)
        check(read(Path('Mount/media/renderscenarios/FrontEnd.xml'))[0] == b'override 60: FrontEnd.xml', 'relative source')
    finally: os.chdir(before_cwd)
    check(read(source, 0xc0000000)[0] == original_bytes[names[0]], 'write-capable requests never redirected')
    override = local / 'data/scene-overrides' / names[0]
    saved = override.read_bytes(); override.unlink()
    data, error = read(source)
    check(data is None and error == 2, 'missing override errors instead of silently using original')
    override.write_bytes(saved)
    source.unlink()
    data, error = read(source)
    check(data is None and error == 2, 'removed source cannot be concealed by override')
    source.write_bytes(original_bytes[names[0]])
    check({p.name:p.read_bytes() for p in scene.iterdir()} == original_bytes, 'all original contents unchanged')


# Each verified host import call must bind the wrapper; all other Win32 calls remain original.
pe = pefile.PE(str(ROOT/'runtime/pc/ForzaMastro5.exe'))
bridge = next(d for d in pe.DIRECTORY_ENTRY_IMPORT if d.dll == b'ForzaMastro5Bridge.dll')
slot = next(i.address-pe.OPTIONAL_HEADER.ImageBase for i in bridge.imports if i.name == b'FM5CreateGameFileW')
sites=[0x2cdbd,0x2f657,0x733d8,0x735bb,0x73726,0x74433,0x76e3d,0xba6df,0xba97b,0xbaca2,0x16c0b3,0x16c104,0x16c766,0x172fe8,0x18d316,0x18d385,0x1a047d,0x221b03,0x24928e,0x249348]
for site in sites:
    instruction=pe.get_data(site,6)
    check(instruction[:2] == b'\xff\x15' and site+6+struct.unpack('<i',instruction[2:])[0] == slot, 'host CreateFileW wrapper import')
report={'passed':True,'checks':checks,'host_routes':len(sites),'scope':'Real native Windows file opens on synthetic fixtures plus host import binding. Actual game scene loading still requires gameplay testing.'}
(ROOT/'analysis/pc-build/scene-file-routing-test.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report))
