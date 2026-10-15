from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,ctypes,sys,hashlib
ROOT=Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile
names={'msvcp110.dll':['??1facet@locale@std@@MEAA@XZ','?_Getcoll@_Locinfo@std@@QEBA?AU_Collvec@@XZ'],
       'msvcr110.dll':['_wtoi64','strtok','strtok_s','wcspbrk']}
report=[]
behavior=[]
for dll,wanted in names.items():
    p=ROOT/'runtime/azimuth-0.9.9-original/data/bin'/dll
    pe=pefile.PE(str(p));exports={s.name.decode():s for s in pe.DIRECTORY_ENTRY_EXPORT.symbols if s.name}
    lib=ctypes.WinDLL(str(p));checks=[]
    for n in wanted:
        s=exports.get(n)
        proc=getattr(lib,n,None)
        checks.append({'name':n,'export_present':s is not None,'rva':hex(s.address) if s else None,
                       'native_GetProcAddress_success':proc is not None})
    report.append({'dll':dll,'sha256':hashlib.sha256(p.read_bytes()).hexdigest(),'checks':checks})
    if dll=='msvcr110.dll':
        toint=lib._wtoi64;toint.argtypes=[ctypes.c_wchar_p];toint.restype=ctypes.c_int64
        assert toint('1234567890123')==1234567890123
        behavior.append({'symbol':'_wtoi64','passed':True})
        tokenize=lib.strtok;tokenize.argtypes=[ctypes.c_void_p,ctypes.c_char_p];tokenize.restype=ctypes.c_char_p
        text=ctypes.create_string_buffer(b'lap;race;pit')
        assert [tokenize(text,b';'),tokenize(None,b';'),tokenize(None,b';'),tokenize(None,b';')]==[b'lap',b'race',b'pit',None]
        behavior.append({'symbol':'strtok','passed':True})
        tokenize_s=lib.strtok_s;tokenize_s.argtypes=[ctypes.c_void_p,ctypes.c_char_p,ctypes.POINTER(ctypes.c_void_p)];tokenize_s.restype=ctypes.c_char_p
        text=ctypes.create_string_buffer(b'lap;race;pit');context=ctypes.c_void_p()
        assert [tokenize_s(text,b';',ctypes.byref(context)),tokenize_s(None,b';',ctypes.byref(context)),tokenize_s(None,b';',ctypes.byref(context)),tokenize_s(None,b';',ctypes.byref(context))]==[b'lap',b'race',b'pit',None]
        behavior.append({'symbol':'strtok_s','passed':True})
        find=lib.wcspbrk;find.argtypes=[ctypes.c_wchar_p,ctypes.c_wchar_p];find.restype=ctypes.c_wchar_p
        assert find('race/lap','/;')=='/lap' and find('race','/;') is None
        behavior.append({'symbol':'wcspbrk','passed':True})
result={'exports':report,'native_behavior_tests':behavior,'scope':'Six exports resolved; four plain CRT functions executed with controlled inputs. C++ locale methods not invoked without real objects.'}
(ROOT/'analysis/audio-tls/crt-export-verification.json').write_text(json.dumps(result,indent=2));print(json.dumps(result,indent=2))
