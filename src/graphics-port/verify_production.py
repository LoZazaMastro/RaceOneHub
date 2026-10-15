from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import ctypes
import hashlib
import json
import subprocess

ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile

OUT = ROOT / 'analysis/graphics-port'
dll_path = OUT / 'fm5_graphics_bridge.dll'
exe_path = ROOT / 'runtime/azimuth-0.9.9-original/azimuth.exe'
dll = ctypes.WinDLL(str(dll_path))
configure = dll.FM5ConfigureGraphicsBridge
configure.argtypes = [ctypes.c_void_p, ctypes.c_uint]
configure.restype = ctypes.c_long
kernel = ctypes.WinDLL('kernel32', use_last_error=True)
kernel.LoadLibraryExW.argtypes = [ctypes.c_wchar_p, ctypes.c_void_p, ctypes.c_uint]
kernel.LoadLibraryExW.restype = ctypes.c_void_p
kernel.FreeLibrary.argtypes = [ctypes.c_void_p]
kernel.FreeLibrary.restype = ctypes.c_int

mapped = kernel.LoadLibraryExW(str(exe_path), None, 1)
if not mapped:
    raise ctypes.WinError(ctypes.get_last_error())
try:
    factory = mapped + 0x58090
    assert configure(factory, 1) == 0
    signature = ctypes.string_at(factory, 256)
    assert configure(factory, 2) < 0
    for offset in range(256):
        changed = bytearray(signature)
        changed[offset] ^= 1
        fixture = ctypes.create_string_buffer(bytes(changed))
        assert configure(ctypes.addressof(fixture), 1) < 0
    assert configure(None, 0) == 0
finally:
    kernel.FreeLibrary(mapped)

pe = pefile.PE(str(dll_path))
assert not getattr(pe, 'DIRECTORY_ENTRY_IMPORT', [])
assert pe.OPTIONAL_HEADER.AddressOfEntryPoint == 0
exports = sorted(x.name.decode() for x in pe.DIRECTORY_ENTRY_EXPORT.symbols)
assert exports == ['FM5ConfigureGraphicsBridge', 'FM5D3D11CreateDevice',
                   'ForzaMastro5D3D11CreateDevice']
result = subprocess.run([str(OUT / 'test_bridge.exe')], capture_output=True, text=True, check=True)
report = {
    'passed': True,
    'dll_sha256': hashlib.sha256(dll_path.read_bytes()).hexdigest(),
    'dll_bytes': dll_path.stat().st_size,
    'exports': exports,
    'imports': [],
    'entrypoint': 0,
    'factory_rva': '0x58090',
    'factory_signature_bytes': len(signature),
    'signature_single_byte_mutation_refusals': 256,
    'test_output': result.stdout.strip(),
    'game_started': False,
    'real_gpu_rendering_tested': False,
    'limitations': [
        'Descriptor flag normalization is an explicit experimental policy, not established semantic equivalence of the two Xbox SDK revisions.',
        'The production function fingerprint complements the root loader whole-host and whole-game guards; it is not whole-image verification.',
        'COM mock tests exercise arguments and reference ownership, not graphics results.',
        'Azimuth graphics preparation must complete before the guest calls the factory.',
        'WARP driver type 5 is deliberately unsupported by this entry bridge.',
    ],
}
(OUT / 'verification.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps(report, indent=2))
