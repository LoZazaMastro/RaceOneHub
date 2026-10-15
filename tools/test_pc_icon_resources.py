"""Validate numerical icons against the actual derived native host, without running it."""
from pathlib import Path
import ctypes as ct
from ctypes import wintypes as wt
import shutil
import struct
import tempfile
from project_paths import add_python_libraries, project_root
add_python_libraries(__file__)
import pefile
from pc_icon_resources import embed_icon_in_place


def check_host(source):
    root = project_root(__file__)
    original = source.read_bytes()
    kernel = ct.WinDLL('kernel32', use_last_error=True)
    user = ct.WinDLL('user32', use_last_error=True)
    kernel.LoadLibraryExW.argtypes = [wt.LPCWSTR, wt.HANDLE, wt.DWORD]
    kernel.LoadLibraryExW.restype = wt.HMODULE
    kernel.FreeLibrary.argtypes = [wt.HMODULE]
    user.LoadImageW.argtypes = [wt.HMODULE, ct.c_void_p, wt.UINT, ct.c_int, ct.c_int, wt.UINT]
    user.LoadImageW.restype = wt.HANDLE
    user.DestroyIcon.argtypes = [wt.HANDLE]
    with tempfile.TemporaryDirectory(prefix='raceone-icon-') as folder:
        for number in ('5',):
            target = Path(folder) / ('RaceOne' + number + '.exe')
            shutil.copy2(source, target)
            ico = root / ('launcher/ForzaMastro.Launcher/Assets/ForzaMotorsport'+number+'.ico')
            report = embed_icon_in_place(target, ico)
            updated = target.read_bytes()
            assert len(updated) == len(original)
            a, b = pefile.PE(data=original), pefile.PE(data=updated)
            assert a.OPTIONAL_HEADER.AddressOfEntryPoint == b.OPTIONAL_HEADER.AddressOfEntryPoint
            assert a.OPTIONAL_HEADER.SizeOfImage == b.OPTIONAL_HEADER.SizeOfImage
            assert [(s.VirtualAddress, s.PointerToRawData, s.SizeOfRawData) for s in a.sections] == [
                (s.VirtualAddress, s.PointerToRawData, s.SizeOfRawData) for s in b.sections]
            icons = next(t for t in b.DIRECTORY_ENTRY_RESOURCE.entries if t.id == 3)
            group = next(t for t in b.DIRECTORY_ENTRY_RESOURCE.entries if t.id == 14)
            entry = group.directory.entries[0]
            resource = entry.directory.entries[0].data.struct
            blob = b.get_data(resource.OffsetToData, resource.Size)
            expected = ico.read_bytes()
            count = report['frames']
            assert struct.unpack_from('<HHH', blob) == (0, 1, count)
            for i in range(count):
                frame = struct.unpack_from('<BBBBHHIH', blob, 6+14*i)
                ico_frame = struct.unpack_from('<BBBBHHII', expected, 6+16*(7-count+i))
                data = next(n for n in icons.directory.entries if n.id == frame[-1]).directory.entries[0].data.struct
                assert frame[:7] == ico_frame[:7]
                assert b.get_data(data.OffsetToData, data.Size) == expected[ico_frame[-1]:ico_frame[-1]+ico_frame[-2]]
            # Map only as resources. DllMain and executable entry point never run.
            module = kernel.LoadLibraryExW(str(target), None, 0x22)
            assert module, ct.get_last_error()
            try:
                for size in (16,24,32,48,64,128,256):
                    handle = user.LoadImageW(module, entry.id, 1, size, size, 0)
                    assert handle, (number, size, ct.get_last_error())
                    user.DestroyIcon(handle)
            finally:
                kernel.FreeLibrary(module)
            print('RaceOne'+number, report, 'Windows resource loading: all 7 sizes PASS')
            a.close(); b.close()
    assert source.read_bytes() == original


def main():
    root = project_root(__file__)
    check_host(root / 'runtime/pc/ForzaMastro5.exe')


if __name__ == '__main__':
    main()
