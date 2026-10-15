from pathlib import Path
import ctypes as C
import hashlib
import json
import os

ROOT = Path(__file__).resolve().parents[1]
class Texture2D(C.Structure):
    _fields_ = [(name, C.c_uint32) for name in (
        'width', 'height', 'mips', 'array_size', 'format', 'sample_count',
        'sample_quality', 'usage', 'bind_flags', 'cpu_access', 'misc_flags', 'esram_offset', 'tile_mode', 'pitch')]

assert C.sizeof(Texture2D) == 56
cases = []
for width, height, mips, array_size in [(1,1,1,1), (4,4,1,1), (16,16,5,1),
    (64,64,7,1), (256,256,9,1), (1920,1080,1,1), (4096,2048,1,1), (256,256,1,6)]:
    for fmt in (28, 71, 77, 87, 98, 99):
        cases.append((width, height, mips, array_size, fmt))
results = {}
with os.add_dll_directory(str(ROOT / 'runtime/xg-host')):
    bridge = C.WinDLL(str(ROOT / 'runtime/xg-host/kernelx.dll'), winmode=0x1100)
    for name in ['xg.dll', 'xg_x.dll']:
        module = C.WinDLL(str(ROOT / 'runtime/xg-host' / name), winmode=0x1100)
        optimal = module.XGComputeOptimalTileMode
        optimal.argtypes = [C.c_uint32] * 7
        optimal.restype = C.c_uint32
        layout = module.XGComputeTexture2DLayout
        layout.argtypes = [C.POINTER(Texture2D), C.c_void_p]
        layout.restype = C.c_int32
        create = module.XGCreateTexture2DComputer
        create.argtypes = [C.POINTER(Texture2D), C.POINTER(C.c_void_p)]
        create.restype = C.c_int32
        records = []
        for values in cases:
            width, height, mips, array_size, fmt = values
            mode = optimal(3, fmt, width, height, array_size, 1, 8)
            desc = Texture2D(width, height, mips, array_size, fmt, 1, 0, 0, 8, 0, 0, 0, mode, 0)
            size = 0xd40
            storage = (C.c_ubyte * (size + 256))(*([0xa5] * (size + 256)))
            address = C.addressof(storage) + 128
            hr = layout(C.byref(desc), address)
            assert hr == 0, (name, values, hex(hr & 0xffffffff))
            assert bytes(storage[:128]) == bytes([0xa5]) * 128
            assert bytes(storage[-128:]) == bytes([0xa5]) * 128
            data = C.string_at(address, size)
            assert data != bytes(size)
            handle = C.c_void_p()
            assert create(C.byref(desc), C.byref(handle)) == 0 and handle.value
            table = C.cast(handle, C.POINTER(C.POINTER(C.c_void_p))).contents
            get_layout = C.WINFUNCTYPE(C.c_int32, C.c_void_p, C.c_void_p)(table[2])
            release = C.WINFUNCTYPE(C.c_uint32, C.c_void_p)(table[1])
            other = C.create_string_buffer(size + 128)
            C.memset(C.addressof(other), 0xa5, size + 128)
            assert get_layout(handle, other) == 0
            assert bytes(other.raw[:size]) == data
            assert bytes(other.raw[size:]) == bytes([0xa5]) * 128
            assert release(handle) == 0
            records.append({'width': width, 'height': height, 'mips': mips, 'array_size': array_size,
                'format': fmt, 'tile_mode': mode, 'layout_sha256': hashlib.sha256(data).hexdigest(),
                'layout_prefix': data[:48].hex(), 'direct_and_factory_layout_identical': True,
                'guards_preserved': True})
        assert layout(None, None) == C.c_int32(0x80070057).value
        handle = C.c_void_p()
        assert create(None, C.byref(handle)) == C.c_int32(0x80070057).value
        assert not handle.value
        results[name] = records
assert results['xg.dll'] == results['xg_x.dll']
report = {'passed': True, 'descriptor_bytes': C.sizeof(Texture2D),
    'layout_interface': 'Original FM5 compact ResourceLayout, exactly 0xd40 bytes; direct export and computer slot 2',
    'valid_layouts_executed': sum(len(v) for v in results.values()),
    'native_factories_executed_and_released': sum(len(v) for v in results.values()),
    'null_arguments_rejected': True, 'both_original_libraries_identical': True, 'results': results,
    'scope': 'Native Windows execution of original FM5 texture layout and factory functions. No game execution or GPU rendering.'}
(ROOT / 'analysis/xg-layout-results.json').write_text(json.dumps(report, indent=2) + '\n')
print(json.dumps({k: v for k,v in report.items() if k != 'results'}))
