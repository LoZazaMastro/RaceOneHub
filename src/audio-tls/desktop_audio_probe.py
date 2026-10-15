from pathlib import Path
import ctypes
import json
import os

ROOT = Path(__file__).resolve().parents[2]
system_dll = Path(os.environ['SystemRoot']) / 'System32/xaudio2_9.dll'
report = {'scope': 'Create and release the installed desktop XAudio 2.9 engine only; no mastering/source voice, sound or game execution.',
          'path': str(system_dll), 'exists': system_dll.is_file()}
try:
    dll = ctypes.WinDLL(str(system_dll))
    create = dll.XAudio2Create
    create.argtypes = [ctypes.POINTER(ctypes.c_void_p), ctypes.c_uint32, ctypes.c_uint32]
    create.restype = ctypes.c_int32
    obj = ctypes.c_void_p()
    hr = create(ctypes.byref(obj), 0, 0)
    report['hresult'] = hex(ctypes.c_uint32(hr).value)
    report['object_created'] = bool(obj.value)
    report['passed'] = hr >= 0 and bool(obj.value)
    if obj.value:
        table = ctypes.cast(obj, ctypes.POINTER(ctypes.POINTER(ctypes.c_void_p))).contents
        release = ctypes.WINFUNCTYPE(ctypes.c_uint32, ctypes.c_void_p)(table[2])
        report['release_reference_count'] = release(obj)
except OSError as exc:
    report['passed'] = False
    report['error'] = str(exc)
(ROOT / 'analysis/audio-tls/desktop-audio-verification.json').write_text(json.dumps(report, indent=2))
print(json.dumps(report, indent=2))
