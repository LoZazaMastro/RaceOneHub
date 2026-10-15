from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import ctypes as C
import hashlib,json,os,struct,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'analysis/allocation-port/language-reference'
add_python_libraries(__file__)
import pefile
game_raw=(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe').read_bytes()
az_raw=(ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe').read_bytes()
assert hashlib.sha256(game_raw).hexdigest()=='94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7'
assert hashlib.sha256(az_raw).hexdigest()=='579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc'
pe=pefile.PE(data=game_raw);game=pe.get_memory_mapped_image();az=pefile.PE(data=az_raw).get_memory_mapped_image()
guards=[('settings_init',0x5d1b31,0x5d1c26),('startup',0x5cfdb4,0x5cfe00),
        ('prefix_lookup',0x7cad10,0x7cad98),('constructor',0x7c7670,0x7ca8c1),
        ('argument_capacity',0x5d2a98,0x5d2ab3),('initial_buffer',0x25d293d,0x25d2945)]
def array(name,data):
 return 'static const BYTE '+name+'[] = {\n'+''.join('    '+','.join('0x'+data[n:n+1].hex() for n in range(i,min(i+24,len(data))))+',\n' for i in range(0,len(data),24))+'};\n'
source='/* Generated only from the two pinned immutable originals. */\n'
source+='typedef struct LanguageGuard { UINT rva; SIZE_T bytes; const BYTE *original; } LanguageGuard;\n'
source+=array('guard_host_locale',az[0x37d00:0x37d6e])+array('guard_host_set_locale',az[0x3abf0:0x3ac16])
for name,start,end in guards:source+=array('guard_'+name,game[start:end])
source+='static const LanguageGuard guest_guards[] = {\n'+''.join(f'    {{0x{s:x},sizeof(guard_{n}),guard_{n}}},\n'for n,s,e in guards)+'};\n'
(ROOT/'src/allocation-port/fm5_language_guards.inc').write_text(source)
vc=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
sdk=Path(r'C:\Program Files (x86)\Windows Kits\10');version='10.0.26100.0';env=os.environ.copy()
env['INCLUDE']=';'.join(map(str,[vc/'include',*[sdk/'Include'/version/p for p in ['ucrt','shared','um','winrt']]]))
env['LIB']=';'.join(map(str,[vc/'lib/x64',*[sdk/'Lib'/version/p/'x64'for p in ['ucrt','um']]]))
cmd=[str(vc/'bin/Hostx64/x64/cl.exe'),'/nologo','/W4','/WX','/O2','/MT','/LD',str(ROOT/'src/allocation-port/fm5_language.c'),'/Fo'+str(OUT)+'\\','/Fe'+str(OUT/'language-test.dll'),'/link','kernel32.lib',
     *['/EXPORT:'+name for name in ['FM5PrepareTextLanguage','FM5CommitTextLanguage','FM5PrepareTextLanguageForLocale','FM5ReadHostSelectedLocaleIndex','FM5TextLanguageLocaleAt']]]
r=subprocess.run(cmd,env=env,capture_output=True,text=True);(OUT/'language.build.log').write_text(r.stdout+r.stderr);assert not r.returncode,r.stdout+r.stderr
dll=C.WinDLL(str(OUT/'language-test.dll'))
class Plan(C.Structure):_fields_=[('site',C.c_void_p),('replacement',C.c_ubyte*7),('index',C.c_uint),('prefix',C.c_char*4)]
prepare=dll.FM5PrepareTextLanguage;prepare.argtypes=[C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t,C.POINTER(Plan)];prepare.restype=C.c_long
commit=dll.FM5CommitTextLanguage;commit.argtypes=[C.POINTER(Plan)]
explicit=dll.FM5PrepareTextLanguageForLocale
explicit.argtypes=[C.c_void_p,C.c_size_t,C.c_void_p,C.c_size_t,C.c_wchar_p,C.c_size_t,C.POINTER(Plan)];explicit.restype=C.c_long
read_index=dll.FM5ReadHostSelectedLocaleIndex;read_index.argtypes=[C.c_void_p,C.c_size_t,C.POINTER(C.c_uint)];read_index.restype=C.c_long
locale_at=dll.FM5TextLanguageLocaleAt;locale_at.argtypes=[C.c_uint];locale_at.restype=C.c_wchar_p
host=C.create_string_buffer(0x3de000);C.memmove(host,az,min(len(az),len(host)))
struct.pack_into('<I',host,struct.unpack_from('<I',az,0x3c)[0]+0x50,len(host))
guest=C.create_string_buffer(game)
def selected(value,heap=False):
 raw=value.encode('utf-16le')+b'\0\0';C.memset(C.addressof(host)+0x31a8b0,0,32)
 if heap:
  storage=C.create_string_buffer(raw);struct.pack_into('<Q',host,0x31a8b0,C.addressof(storage));capacity=64
 else:C.memmove(C.addressof(host)+0x31a8b0,raw,len(raw));capacity=7;storage=None
 struct.pack_into('<QQ',host,0x31a8c0,len(value),capacity)
 return storage
def prep():
 plan=Plan();hr=prepare(host,len(host),guest,len(game),C.byref(plan));return hr,plan
kernel=C.WinDLL('kernel32');kernel.VirtualAlloc.argtypes=[C.c_void_p,C.c_size_t,C.c_uint,C.c_uint];kernel.VirtualAlloc.restype=C.c_void_p
kernel.VirtualFree.argtypes=[C.c_void_p,C.c_size_t,C.c_uint]
memory=kernel.VirtualAlloc(None,pe.OPTIONAL_HEADER.SizeOfImage,0x3000,0x40);assert memory
C.memmove(memory,game,len(game))
copy_records=[];hash_records=[]
@C.WINFUNCTYPE(C.c_int,C.c_void_p,C.c_size_t,C.c_void_p)
def copy_string(dest,size,src):
 text=C.string_at(src);assert len(text)<size;C.memmove(dest,text+b'\0',len(text)+1);copy_records.append((dest,size,text.decode()));return 0
@C.WINFUNCTYPE(C.c_int,C.c_int)
def isalpha(value):return int(65<=value<=90 or 97<=value<=122)
@C.WINFUNCTYPE(C.c_int,C.c_int)
def isupper(value):return int(65<=value<=90)
@C.WINFUNCTYPE(C.c_int,C.c_int)
def tolower(value):return value+32 if 65<=value<=90 else value
@C.WINFUNCTYPE(C.c_void_p,C.c_void_p,C.c_void_p,C.c_uint,C.c_ubyte)
def hash_record(dest,source,characters,flag):
 assert characters==16 and flag==0;hash_records.append(dest);return dest
@C.WINFUNCTYPE(None,C.c_void_p)
def locale_record(manager):C.c_uint.from_address(manager+4).value=0
@C.WINFUNCTYPE(C.c_int,C.c_void_p,C.c_void_p,C.c_size_t)
def compare(left,right,size):
 l=C.string_at(left).lower()[:size];r=C.string_at(right).lower()[:size];return (l>r)-(l<r)
@C.WINFUNCTYPE(C.c_void_p,C.c_void_p,C.c_int,C.c_size_t)
def fill_bytes(dest,value,size):C.memset(dest,value,size);return dest
def jump(rva,callback):C.memmove(memory+rva,b'\x48\xb8'+struct.pack('<Q',C.cast(callback,C.c_void_p).value)+b'\xff\xe0',12)
for offset,cb in [(0x101b848,copy_string),(0x101b908,isalpha),(0x101b910,isupper),(0x101ba90,tolower),(0x101b8b0,compare)]:
 C.c_void_p.from_address(memory+offset).value=C.cast(cb,C.c_void_p).value
# Resolve the real original lookup's single CRT import, without changing its body.
lookup_import=0x7cad50+struct.unpack_from('<i',game,0x7cad4c)[0]
C.c_void_p.from_address(memory+lookup_import).value=C.cast(compare,C.c_void_p).value
jump(0x7ee2a0,hash_record);jump(0x7ca990,locale_record)
jump(0xca8180,fill_bytes)
manager=C.create_string_buffer(0x1270)
construct=C.WINFUNCTYPE(None,C.c_void_p)(memory+0x7c7670)
construct(manager)
assert len(hash_records)==17
prefixes=[C.string_at(C.addressof(manager)+0x10+i*0xe0).decode()for i in range(17)]
assert prefixes==['EN','EN','GB','JP','DE','FR','MX','ES','IT','KO','CHT','BR','CZ','HU','PL','RU','NL'],prefixes
lookup=C.WINFUNCTYPE(C.c_uint,C.c_void_p,C.c_char_p)(memory+0x7cad10)
settings=C.create_string_buffer(0x3000)
initialize_settings=C.WINFUNCTYPE(None,C.c_void_p)(memory+0x5d1ae0)
cases=[]
for code,index,prefix in [('en-US',0,'EN'),('en-GB',2,'GB'),('ja-JP',3,'JP'),('de-DE',4,'DE'),('fr-FR',5,'FR'),('es-MX',6,'MX'),('es-ES',7,'ES'),('it-IT',8,'IT'),('ko-KR',9,'KO'),('zh-TW',10,'CHT'),('pt-BR',11,'BR'),('cs-CZ',12,'CZ'),('hu-HU',13,'HU'),('pl-PL',14,'PL'),('ru-RU',15,'RU'),('nl-NL',16,'NL')]:
 for heap in [False,True]:
  storage=selected(code,heap);hr,plan=prep();assert hr==0,(code,hr)
  assert plan.index==index and plan.prefix.decode()==prefix
  assert guest[0x5d1b38:0x5d1b3f]==game[0x5d1b38:0x5d1b3f]
  commit(C.byref(plan));patched=guest[0x5d1b38:0x5d1b3f]
  target=0x5d1b3f+struct.unpack_from('<i',patched,3)[0]
  assert game[target:target+len(prefix)+1]==prefix.encode()+b'\0'
  C.memmove(memory+0x5d1b38,patched,7)
  initialize_settings(settings)
  actual_prefix=C.string_at(C.addressof(settings)+0x248d)
  assert actual_prefix==prefix.encode()
  assert lookup(manager,actual_prefix)==index
  C.memmove(C.addressof(guest)+0x5d1b38,game[0x5d1b38:0x5d1b3f],7)
 cases.append({'locale':code,'prefix':prefix,'original_lookup_enum':index,'inline_and_heap_host_string':True})
selected('');hr,plan=prep();assert hr==1 and not plan.site;commit(C.byref(plan));assert guest.raw[:len(game)]==game
C.memmove(memory+0x5d1b38,game[0x5d1b38:0x5d1b3f],7);initialize_settings(settings)
assert C.string_at(C.addressof(settings)+0x248d)==b'auto'
selected('it-IT');refusals=0
for name,start,end in guards:
 for offset in range(start,end):
  original=guest[offset];guest[offset]=bytes([original[0]^1]);before=guest[0x5d1b38:0x5d1b3f]
  hr,plan=prep();assert hr<0 and not plan.site
  assert guest[0x5d1b38:0x5d1b3f]==before
  guest[offset]=original;refusals+=1
for offset in [*range(0x37d00,0x37d6e),*range(0x3abf0,0x3ac16)]:
 value=host[offset];host[offset]=bytes([value[0]^1]);hr,plan=prep();assert hr<0 and not plan.site;host[offset]=value;refusals+=1
for start,size in [(0x1cbf064,3),(0x1cbf060,3),(0x1cbf06c,3),(0x1cbf068,3),(0x1cbf054,3),
                   (0x1cbf050,3),(0x1cbf05c,3),(0x1cbf058,3),(0x1cbf044,3),(0x1cbf040,4),
                   (0x1cbf038,3),(0x1cbf04c,3),(0x1cbf048,3),(0x1cbf034,3),(0x1cbf030,3),(0x1cbf03c,3)]:
 for offset in range(start,start+size):
  value=guest[offset];guest[offset]=bytes([value[0]^1]);hr,plan=prep();assert hr<0 and not plan.site
  guest[offset]=value;refusals+=1
for value in ['System','system','SYSTEM']:
 selected(value);assert prep()[0]==1
selected('zz-ZZ');assert prep()[0]<0
selected('it-IT');struct.pack_into('<Q',host,0x31a8c8,1);assert prep()[0]<0
selected('it-IT',True);struct.pack_into('<Q',host,0x31a8b0,1);assert prep()[0]<0
selected('it-IT');host[0x31a8b0+10]=b'X';assert prep()[0]<0
native_size=len(host)
native=kernel.VirtualAlloc(None,native_size,0x3000,0x40);assert native
C.memmove(native,az,min(len(az),native_size));C.c_uint.from_address(native+struct.unpack_from('<I',az,0x3c)[0]+0x50).value=native_size
C.memset(native+0x31a8b0,0,32);C.c_size_t.from_address(native+0x31a8c8).value=7
@C.WINFUNCTYPE(C.c_void_p,C.c_void_p,C.c_void_p,C.c_size_t)
def move_bytes(dest,source,size):C.memmove(dest,source,size);return dest
C.memmove(native+0x24b6f0,b'\x48\xb8'+struct.pack('<Q',C.cast(move_bytes,C.c_void_p).value)+b'\xff\xe0',12)
setter=C.WINFUNCTYPE(C.c_void_p,C.c_void_p)(native+0x3abf0)
native_index=C.c_uint(123);assert read_index(native,native_size,C.byref(native_index))==1 and native_index.value==0
prepared=[]
for ordinal in range(16):
 code=locale_at(ordinal);assert code==cases[ordinal]['locale']
 plan=Plan();assert explicit(native,native_size,guest,len(game),code,len(code),C.byref(plan))==0
 prepared.append(plan)
assert locale_at(16) is None
# After early guard validation, the late reader must not inspect guest memory.
old_guard=guest[0x7c7670];guest[0x7c7670]=bytes([old_guard[0]^1])
late_checks=0
for ordinal,case in enumerate(cases):
 code=case['locale']
 for heap in [False,True]:
  argument=C.create_string_buffer(32);raw=code.encode('utf-16le')+b'\0\0'
  if heap:
   storage=C.create_string_buffer(raw);struct.pack_into('<Q',argument,0,C.addressof(storage));capacity=64
  else:C.memmove(argument,raw,len(raw));capacity=7
  struct.pack_into('<QQ',argument,16,len(code),capacity)
  assert setter(argument)==native+0x31a8b0
  assert read_index(native,native_size,C.byref(native_index))==0 and native_index.value==case['original_lookup_enum']
  cached=next(p for p in prepared if p.index==native_index.value)
  assert cached.prefix.decode()==case['prefix']
  commit(C.byref(cached))
  C.memmove(memory+0x5d1b38,guest[0x5d1b38:0x5d1b3f],7);initialize_settings(settings)
  assert lookup(manager,C.string_at(C.addressof(settings)+0x248d))==cached.index
  C.memmove(C.addressof(guest)+0x5d1b38,game[0x5d1b38:0x5d1b3f],7);late_checks+=1
empty=C.create_string_buffer(32);struct.pack_into('<Q',empty,24,7)
guest[0x7c7670]=old_guard
alias_checks=0
for code in ['zh-HK','zh-hk','ZH-HK']:
 selected(code);hr,plan=prep();assert hr==0 and plan.index==10 and plan.prefix==b'CHT'
 assert explicit(native,native_size,guest,len(game),code,len(code),C.byref(plan))==0 and plan.index==10
 argument=C.create_string_buffer(32);raw=code.encode('utf-16le')+b'\0\0';C.memmove(argument,raw,len(raw))
 struct.pack_into('<QQ',argument,16,len(code),7);assert setter(argument)==native+0x31a8b0
 assert read_index(native,native_size,C.byref(native_index))==0 and native_index.value==10
 alias_checks+=1
assert setter(empty)==native+0x31a8b0
assert read_index(native,native_size,C.byref(native_index))==1 and native_index.value==0
assert kernel.VirtualFree(native,0,0x8000)
assert kernel.VirtualFree(memory,0,0x8000)
report={'passed':True,'cases':cases,'system_unchanged':True,'independent_guard_byte_refusals':refusals,
 'original_constructor_executed':True,'patched_original_settings_constructor_executed':True,
 'constructor_typed_recorders':{'crt_copy_calls':len(copy_records),'hash_records':len(hash_records),'locale_query_stubbed_to_system_enum0':True},
 'original_prefix_lookup_body_executed_unmodified':True,'startup_field_bytes_proven':8,
 'early_precomputed_plans':len(prepared),'genuine_original_native_setter_late_selection_cases':late_checks,
 'native_setter_return_pointer_exact':True,'late_host_reader_ignores_intentionally_invalid_guest_guard':True,
 'legacy_zh_HK_alias_cases':alias_checks,
 'mutation':'Only4byte RIP displacement in original7byte LEA R8 at5d1b38; constructor usesrealexisting8byte field andstrcpy_s',
 'no_game_process_launched':True,'runtime_files_changed':False,'voice_override_implemented':False}
(OUT/'language-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps({'passed':True,'locales':len(cases),'guard_refusals':refusals,'constructor_records':len(hash_records)}))
