"""Isolated CPU and real hardware eight-sample verification; no game launch."""
from pathlib import Path
import ctypes as C, hashlib, json, struct, sys
ROOT=Path(__file__).resolve().parents[2]
base=(ROOT/'src/allocation-port/build_msaa4_tests.py').read_text().split("game = ROOT/")[0]
exec(compile(base,'build-definitions','exec'))
module=ROOT/'src/allocation-port/fm5_msaa4.c'
compile('msaa8-selector-test.dll',[module],['/LD','/DFM5_MSAA4_TEST'],['/EXPORT:FM5SelectFourSampleResolve','/EXPORT:FM5SetMsaaTestHost','/EXPORT:FM5ForcedSampleCount','/EXPORT:FM5MapNativeSampleMask','/EXPORT:FM5ParseNativeAntialiasing'])
dll=C.WinDLL(str(OUT/'msaa8-selector-test.dll'))
select=dll.FM5SelectFourSampleResolve
select.argtypes=[C.c_void_p,C.c_size_t,C.c_uint,C.POINTER(C.c_void_p),C.POINTER(C.c_size_t)];select.restype=C.c_long
dll.FM5SetMsaaTestHost.argtypes=[C.c_void_p]
mask=dll.FM5MapNativeSampleMask;mask.argtypes=[C.c_uint]*4;mask.restype=C.c_uint
parse=dll.FM5ParseNativeAntialiasing;parse.argtypes=[C.c_void_p,C.c_uint];parse.restype=C.c_uint
host=C.create_string_buffer(0x319000)
struct.pack_into('<H',host,0,0x5a4d);struct.pack_into('<I',host,0x3c,0x80);struct.pack_into('<I',host,0x80,0x4550);struct.pack_into('<H',host,0x98,0x20b);struct.pack_into('<I',host,0xd0,0x319000)
for rva in [0xe752a,0x130a4e,0x132310,0x1a7e60,0x1ab026]:C.memset(C.addressof(host)+rva,0x90,2)
C.memset(C.addressof(host)+0xe7551,0x90,6);struct.pack_into('<I',host,0x318ea0,8)
dll.FM5SetMsaaTestHost(host);assert dll.FM5ForcedSampleCount()==8
for rva,length in [(x,2)for x in [0xe752a,0x130a4e,0x132310,0x1a7e60,0x1ab026]]+[(0xe7551,6)]:
 for n in range(length):
  byte=C.c_ubyte.from_address(C.addressof(host)+rva+n);byte.value^=1;assert dll.FM5ForcedSampleCount()==0;byte.value^=1
for heap in [False,True]:
 for text,expected in [('off',1),('msaax2',2),('msaax4',4),('msaax8',8),('MSAAX8',99),('msaax6',99),('msaax80',99),('',99)]:
  storage=C.create_string_buffer(32);wstring=C.create_unicode_buffer(text)
  if heap:struct.pack_into('<Q',storage,0,C.addressof(wstring));cap=32
  else:C.memmove(storage,wstring,min(len(text)*2,16));cap=7
  struct.pack_into('<QQ',storage,16,len(text),cap);assert parse(storage,99)==expected,(heap,text)
# Execute the actual production host replacement on RX pages with the
# original native stack locations (RBP+20 wchar string, R13+4 UINT).
sys.path.insert(0,str(ROOT/'tools'));from fm5_msaa_hook import antialiasing_reader_code
k=C.WinDLL('kernel32',use_last_error=True)
k.VirtualAlloc.argtypes=[C.c_void_p,C.c_size_t,C.c_uint,C.c_uint];k.VirtualAlloc.restype=C.c_void_p
k.VirtualProtect.argtypes=[C.c_void_p,C.c_size_t,C.c_uint,C.POINTER(C.c_uint)]
k.VirtualFree.argtypes=[C.c_void_p,C.c_size_t,C.c_uint]
page=k.VirtualAlloc(None,4096,0x3000,4);assert page
entry=bytes.fromhex('5541554883ec28488d6ae04c8d6c241c41894d04e9')+struct.pack('<i',0x100-25)
# The jump's next address is25; verify this independently from assembler bytes.
assert len(entry)==25
C.memmove(page,entry,len(entry));code=antialiasing_reader_code(page+0x100,page+0x210,page+0x400);C.memmove(page+0x100,bytes(code),len(code))
tail=bytes.fromhex('418b45044883c428415d5dc3');C.memmove(page+0x210,tail,len(tail))
C.memmove(page+0x400,bytes.fromhex('ff25fa000000'),6);C.memmove(page+0x500,struct.pack('<Q',C.cast(parse,C.c_void_p).value),8)
old=C.c_uint();assert k.VirtualProtect(page,4096,0x20,C.byref(old))
invoke=C.WINFUNCTYPE(C.c_uint,C.c_uint,C.c_void_p)(page)
for text,expected in [('off',1),('msaax2',2),('msaax4',4),('msaax8',8),('unknown',99)]:
 storage=C.create_string_buffer(32);wstring=C.create_unicode_buffer(text);C.memmove(storage,wstring,len(text)*2);struct.pack_into('<QQ',storage,16,len(text),7)
 assert invoke(99,storage)==expected,(text,expected)
assert k.VirtualFree(page,0,0x8000)
# Derive the 4-to8 partition from Microsoft's standard positions, not a
# second copy of the C masks. Every nearest neighbour is unambiguous.
p4=[(-2,-6),(6,-2),(-6,2),(2,6)];p8=[(1,-3),(-1,3),(5,1),(-3,-5),(-5,5),(-7,-1),(3,7),(7,-7)]
regions=[]
for p in p8:
 distances=[(p[0]-q[0])**2+(p[1]-q[1])**2 for q in p4]
 assert distances.count(min(distances))==1;regions.append(distances.index(min(distances)))
for bits in range(16):
 expected=sum(1<<n for n,owner in enumerate(regions)if bits&(1<<owner))
 assert mask(8,2,2,bits)==expected
for bits in range(256):assert mask(8,4,3,bits)==bits
assert mask(8,2,1,1)==0xc6 and mask(8,2,1,2)==0x39
for count,quality in [(2,1),(2,2),(4,3),(8,3)]:assert mask(8,count,quality,0xffffffff)==0xffffffff
families=[]
for index,rva in enumerate([0x11d41d0,0x11d9180,0x14a6140,0x14ab738,0x171a050,0x171ff0c,0x18a68b0,0x18aceb0]):
 original=(OUT/f'original-{rva:x}.dxbc').read_bytes();buffer=C.create_string_buffer(original);pointer=C.c_void_p();size=C.c_size_t()
 assert select(buffer,len(original),8,C.byref(pointer),C.byref(size))==0
 data=C.string_at(pointer,size.value)
 def chunks(b):
  d={}
  for n in range(struct.unpack_from('<I',b,28)[0]):
   off=struct.unpack_from('<I',b,32+n*4)[0];tag,length=struct.unpack_from('<4sI',b,off);d[tag]=b[off+8:off+8+length]
  return d
 a,b=chunks(original),chunks(data)
 assert a[b'ISGN']==b[b'ISGN'] and a[b'OSGN']==b[b'OSGN']
 assert b'TexFMask' not in b[b'RDEF']
 (OUT/('selected-production-eight-samples'+('-tonemap' if index>=4 else '')+'.dxbc')).write_bytes(data)
 families.append(dict(rva=hex(rva),bytes=len(data),sha256=hashlib.sha256(data).hexdigest()))
 assert select(buffer,len(original),2,C.byref(pointer),C.byref(size))==1 and pointer.value==C.addressof(buffer)
compile('test-msaa8-fmask-interop.exe',[module,ROOT/'src/allocation-port/test_msaa4_fmask_interop.c',ROOT/'src/graphics-port/fm5_fmask_bridge.c',ROOT/'src/audio-tls/resource-port/fm5_resource.c',ROOT/'src/allocation-port/fm5_allocation.c'],['/DFM5_MSAA4_TEST','/DFM5_FMASK_TEST','/DFM5_RESOURCE_TEST'])
interop,interop_errors=execute([OUT/'test-msaa8-fmask-interop.exe'])
compile('probe-eight-sample-mask.exe',[module,OUT/'probe-eight-sample-mask.cpp'],['/EHsc','/DFM5_MSAA4_TEST','/I'+str(module.parent)],['d3d11.lib','d3dcompiler.lib'])
identity,identity_errors=execute([OUT/'probe-eight-sample-mask.exe',OUT/'selected-production-eight-samples.dxbc'])
assert len(identity.splitlines())==277
compile('probe-tonemap-eight-samples.exe',[OUT/'probe-tonemap-eight-samples.cpp'],['/EHsc'],['d3d11.lib','d3dcompiler.lib'])
tonemap,tonemap_errors=execute([OUT/'probe-tonemap-eight-samples.exe',OUT/'selected-production-eight-samples-tonemap.dxbc'])
report=dict(passed=True,shader_families=families,identity_gpu_cases=277,tonemap_gpu_cases=3,parser_inline_heap_cases=16,production_parser_rx_cases=5,normalizer_byte_guard_refusals=16,identity_gpu_stdout=identity,tonemap_gpu_stdout=tonemap,stderr=identity_errors+tonemap_errors,source_standard_positions='https://microsoft.github.io/DirectX-Specs/d3d/archive/D3D11_3_FunctionalSpec.htm',fmask_interop_stdout=interop,fmask_r8_policy='Legacy typed binding only; all exact guarded forced8 resolvers use direct8 physical loads; unknown family rejected',game_launched=False,runtime_modified=False,native_az_integration='Root-owned native physical-resource/PSO probe still required')
(OUT/'complete-msaa8-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps({k:report[k]for k in ['passed','identity_gpu_cases','tonemap_gpu_cases']}))
