"""Regenerate private FM5 adapter inputs from explicitly supplied originals.

This recipe contains offsets, digests and authored transformation code, not
original code/shader payloads. Generated output is user-local and must never
be included in a public source or binary archive.
"""
from __future__ import annotations
import argparse
import ctypes as C
import hashlib
import json
import math
import os
from pathlib import Path
import shutil
import struct
import tempfile
import pefile

GAME_SHA='94dad21851d335e9a33231dc2a1de2ecd4b93eefdf5f6772085bf6886bf47db7'
ENGINE_SHA='579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc'
# The two original engine locale routines. The prepared host keeps them
# unchanged, so either the original engine or the packaged host supplies them.
HOST_GUARDS={'guard_host_locale':(0x37d00,0x37d6e,'2f93f07827a23c34e4c39c915365824485ac268c74c558b9a411afb980ee5814'),
 'guard_host_set_locale':(0x3abf0,0x3ac16,'0485868ebc10433cd716f52750ea35b848d6d5391fff3c30a40d79ce46afbedf')}
EXPECTED_COMPILED={
 'fm5_resolve_four_samples':(1004,'e789a998553c5b626e9434989cded1df23836fc7c94bdb0ac7c8f436d0f7ab86'),
 'fm5_resolve_four_samples_tonemap':(4484,'c2dc26e17997a5a6637bdf07234fc378ddbc849e14f92e4a1426c4d6b1361846'),
 'fm5_resolve_eight_samples':(1292,'a40988e6afaa828d47169eb58bb0610f4108bd45fa9961c6752ad823560e9b1a'),
 'fm5_resolve_eight_samples_tonemap':(5924,'4b09117d795ab56189fca4911519b4b9e8a9eb94cb54f4bbac479eb6d11494af'),
}
LANGUAGE_GUARDS=[('settings_init',0x5d1b31,0x5d1c26),('startup',0x5cfdb4,0x5cfe00),
 ('prefix_lookup',0x7cad10,0x7cad98),('constructor',0x7c7670,0x7ca8c1),
 ('argument_capacity',0x5d2a98,0x5d2ab3),('initial_buffer',0x25d293d,0x25d2945)]
SHADERS=[0x11d41d0,0x14a6140,0x171a050,0x18a68b0]

def digest(value):return hashlib.sha256(value).hexdigest()
def u32(data,offset):
 if offset<0 or offset+4>len(data):raise ValueError('Truncated DWORD')
 return struct.unpack_from('<I',data,offset)[0]

def chunks(data):
 if len(data)<32 or data[:4]!=b'DXBC' or u32(data,24)!=len(data):raise ValueError('Invalid DXBC container')
 count=u32(data,28)
 if count>32 or 32+4*count>len(data):raise ValueError('Invalid DXBC chunk table')
 result={}
 for i in range(count):
  offset=u32(data,32+4*i)
  if offset<32+4*count or offset+8>len(data):raise ValueError('Invalid DXBC chunk offset')
  n=u32(data,offset+4);tag=data[offset:offset+4]
  if tag in result or offset+8+n>len(data):raise ValueError('Invalid DXBC chunk extent')
  result[tag]=data[offset+8:offset+8+n]
 return result

def shader_at(pe,rva):
 header=pe.get_data(rva,32);n=u32(header,24)
 if not 32<=n<=1048576:raise ValueError('Invalid shader extent')
 data=pe.get_data(rva,n)
 if len(data)!=n:raise ValueError('Truncated shader')
 return chunks(data)

def msf_streams(data):
 if len(data)<56 or not data.startswith(b'Microsoft C/C++ MSF 7.00\r\n\x1aDS\0\0\0'):raise ValueError('Unsupported private shader debug format')
 block,free,count,directory_bytes,reserved,map_block=struct.unpack_from('<6I',data,32)
 if block not in [512,1024,2048,4096]or count*block!=len(data)or directory_bytes>len(data):raise ValueError('Invalid MSF bounds')
 def blocks(numbers):
  if any(n>=count for n in numbers):raise ValueError('MSF block outside input')
  return b''.join(data[n*block:(n+1)*block]for n in numbers)
 map_count=math.ceil(directory_bytes/block)
 if map_block>=count or map_count*4>block:raise ValueError('Invalid MSF directory map')
 directory=blocks(struct.unpack_from('<%dI'%map_count,data,map_block*block))[:directory_bytes]
 streams=u32(directory,0)
 if streams>4096 or 4+4*streams>len(directory):raise ValueError('Invalid MSF stream table')
 sizes=struct.unpack_from('<%dI'%streams,directory,4);offset=4+4*streams;result=[]
 for n in sizes:
  if n==0xffffffff:result.append(None);continue
  if n>len(data):raise ValueError('Invalid MSF stream size')
  count_blocks=math.ceil(n/block)
  if offset+count_blocks*4>len(directory):raise ValueError('Truncated MSF stream blocks')
  ids=struct.unpack_from('<%dI'%count_blocks,directory,offset);offset+=count_blocks*4
  result.append(blocks(ids)[:n])
 return result

def replace_main(source,replacement):
 import re
 matches=list(re.finditer(r'\bfloat4\s+main\s*\(',source))
 if len(matches)!=1:raise ValueError('Expected one original pixel entry point')
 start=matches[0].start();brace=source.find('{',matches[0].end())
 if brace<0:raise ValueError('Missing entry body')
 depth=1;end=brace+1
 while end<len(source)and depth:
  if source[end]=='{':depth+=1
  elif source[end]=='}':depth-=1
  end+=1
 if depth:raise ValueError('Unclosed original shader entry point')
 return source[:start]+replacement+source[end:]

def authored_identity(count):
 # Standalone ABI declaration and averaging: no original shader source here.
 source='// Exact original2Frag4Samp has identity TonemapFragment/UntonemapFragment.\n'
 source+='Texture2DMS<float4> TexColorMSAA : register(t0);\nstruct PS_INPUT {\n float4 PosVS : SV_POSITION;\n float4 UvVS : TexCoord0;\n float4 VigTex_Filmic : TexCoord1;\n float4 FilmicInv1 : TexCoord2;\n float4 FilmicInv2 : TexCoord3;\n};\n'
 source+='float4 main(PS_INPUT input) : SV_TARGET0 {\n int2 pixel = int2(input.PosVS.xy);\n float4 color = TexColorMSAA.Load(pixel, 0);\n'
 source+=''.join(f' color += TexColorMSAA.Load(pixel, {i});\n'for i in range(1,count))
 return source+f' return color * {1/count:g}f;\n}}\n'

def transformed_tonemap(streams,count):
 # Sources are recovered only from the selected owner's pinned input. Preserve
 # original constants/CB bindings and every tone/inverse-tone function; replace
 # the compressed-FMASK accumulation entry point with all physical samples.
 shader,image,color=(streams[i].decode('utf-8').replace('\r\n','\n')for i in [8,9,10])
 if shader.count('#include "ImageProcessor.sh"')!=1 or image.count('#include "ColorSpace.sh"')!=1:raise ValueError('Unexpected original shader include graph')
 shader=shader.replace('#include "ImageProcessor.sh"','')
 image=image.replace('#include "ColorSpace.sh"','')
 entry=('float4 main(PS_INPUT input) : SV_TARGET0 {\n int2 pixel = int2(input.PosVS.xy);\n float4 sum = 0;\n'
  f' [unroll] for(uint i = 0; i < {count}; ++i) {{\n'
  '  float4 fragment = TexColorMSAA.Load(pixel, i);\n'
  '  sum += float4(TonemapFragment(fragment.rgb, input), fragment.a);\n }\n'
  f' return float4(UntonemapFragment(sum.rgb * {1/count:g}f, input), sum.a * {1/count:g}f);\n}}\n')
 return color+'\n'+image+'\n'+replace_main(shader,entry)

def compile_shader(source,compiler,flags):
 dll=C.WinDLL(str(compiler));P=C.c_void_p;H=C.c_long;U=C.c_uint
 fn=dll.D3DCompile;fn.argtypes=[P,C.c_size_t,C.c_char_p,P,P,C.c_char_p,C.c_char_p,U,U,C.POINTER(P),C.POINTER(P)];fn.restype=H
 source_bytes=source.encode('utf-8');data=C.create_string_buffer(source_bytes);code=P();error=P()
 hr=fn(data,len(source_bytes),b'fm5-fmask-probe',None,None,b'main',b'ps_5_0',flags,0,C.byref(code),C.byref(error))
 def blob(pointer):
  if not pointer:return b''
  vt=C.cast(pointer,C.POINTER(C.POINTER(P))).contents
  get=C.WINFUNCTYPE(P,P)(vt[3]);size=C.WINFUNCTYPE(C.c_size_t,P)(vt[4]);release=C.WINFUNCTYPE(U,P)(vt[2])
  try:return C.string_at(get(pointer),size(pointer))
  finally:release(pointer)
 message=blob(error);result=blob(code)
 if hr<0:raise RuntimeError('Pixel shader compilation failed: '+message.decode('utf-8',errors='replace')[:4000])
 return result

def array(name,data):
 return 'static const BYTE '+name+'[] = {\n'+''.join('    '+','.join('0x%02x'%n for n in data[i:i+24])+',\n'for i in range(0,len(data),24))+'};\n'

def checked_input(path,expected,label):
 path=Path(path).expanduser()
 if not path.is_absolute()or not path.is_file():raise ValueError(label+' must be an explicit existing absolute file')
 raw=path.read_bytes()
 if digest(raw)!=expected:raise ValueError(label+' original SHA-256 mismatch')
 return path.resolve(),raw,pefile.PE(data=raw)

def checked_output(path,inputs):
 path=Path(path).expanduser()
 if not path.is_absolute():raise ValueError('Output must be an explicit absolute directory')
 for p in [path,*path.parents]:
  if p.exists()and (p.is_symlink()or getattr(p.lstat(),'st_file_attributes',0)&0x400):raise ValueError('Output cannot traverse a reparse point')
 path=path.resolve()
 for source in inputs:
  owner=source.parent.parent if source.parent.name.casefold()=='mount'else source.parent
  if path==owner or owner in path.parents or path==source or path in source.parents:raise ValueError('Output must stay outside the original input directories')
 if not path.parent.is_dir():raise ValueError('Output parent directory must already exist')
 if path.exists()and (not path.is_dir()or any(path.iterdir())):raise ValueError('Output must be absent or empty; nothing will be overwritten')
 return path

def checked_host(path):
 path=Path(path).expanduser()
 if not path.is_absolute()or not path.is_file():raise ValueError('Prepared host must be an explicit existing absolute file')
 raw=path.read_bytes();pe=pefile.PE(data=raw)
 for name,(start,end,expected)in HOST_GUARDS.items():
  if digest(pe.get_data(start,end-start))!=expected:raise ValueError('Prepared host does not retain the original '+name)
 return path.resolve(),raw,pe

def generate(game_exe:Path,engine_exe:Path|None,output_directory:Path,compiler_path:Path|None=None,prepared_host:Path|None=None)->dict:
 game_path,game_raw,game=checked_input(game_exe,GAME_SHA,'FM5 game executable')
 if(engine_exe is None)==(prepared_host is None):raise ValueError('Supply either the original engine or the prepared host')
 if engine_exe is not None:
  engine_path,engine_raw,engine=checked_input(engine_exe,ENGINE_SHA,'Original external engine')
  inputs={'game':GAME_SHA,'engine':ENGINE_SHA}
 else:
  engine_path,engine_raw,engine=checked_host(prepared_host)
  inputs={'game':GAME_SHA,'prepared_host':digest(engine_raw)}
 out=checked_output(output_directory,[game_path,engine_path])
 if os.name!='nt':raise RuntimeError('Windows D3DCompiler is required for native shader generation')
 compiler=Path(compiler_path)if compiler_path is not None else Path(os.environ['SystemRoot'])/'System32/d3dcompiler_47.dll'
 if not compiler.is_absolute()or not compiler.is_file():raise ValueError('Compiler must be an explicit existing absolute DLL file')
 records=[];all_arrays={};outputs={};prefix='/* Generated locally from explicit pinned owner inputs; do not redistribute. */\n'
 def add(name,data):
  if name in all_arrays:raise ValueError('Duplicate generated symbol')
  all_arrays[name]=data;records.append({'name':name,'bytes':len(data),'sha256':digest(data)})
  return array(name,data)
 text=prefix+'typedef struct LanguageGuard { UINT rva; SIZE_T bytes; const BYTE *original; } LanguageGuard;\n'
 host=engine.get_memory_mapped_image();guest=game.get_memory_mapped_image()
 text+=add('guard_host_locale',host[0x37d00:0x37d6e])+add('guard_host_set_locale',host[0x3abf0:0x3ac16])
 for name,start,end in LANGUAGE_GUARDS:text+=add('guard_'+name,guest[start:end])
 text+='static const LanguageGuard guest_guards[] = {\n'+''.join(f'    {{0x{s:x},sizeof(guard_{name}),guard_{name}}},\n'for name,s,e in LANGUAGE_GUARDS)+'};\n'
 outputs['fm5_language_guards.inc']=text.encode()
 text=prefix;family={}
 for rva in SHADERS:
  cs=shader_at(game,rva);family[rva]=cs
  for tag in [b'RDEF',b'ISGN',b'OSGN',b'SHEX']:
   if tag not in cs:raise ValueError('Missing expected original shader semantic chunk')
   text+=add(f'original_{rva:x}_{tag.decode().lower()}',cs[tag])
 outputs['fm5_resolve_original_guard.inc']=text.encode()
 streams=msf_streams(family[0x171a050][b'SPDB'])
 for count,word in [(4,'four'),(8,'eight')]:
  text=prefix
  for tone in [False,True]:
   name=f'fm5_resolve_{word}_samples'+('_tonemap'if tone else'')
   source=transformed_tonemap(streams,count)if tone else authored_identity(count)
   # Exact production recipes differ:4x strict+O3;8x strict+defaultO1.
   flags=(1<<11)|((1<<15)if count==4 else 0)
   data=compile_shader(source,compiler,flags);expected_bytes,expected_sha=EXPECTED_COMPILED[name]
   if len(data)!=expected_bytes or digest(data)!=expected_sha:raise ValueError('Generated shader differs from the GPU-validated baseline: '+name+' '+str(len(data))+' '+digest(data))
   cs=chunks(data);original=family[0x171a050 if tone else 0x11d41d0]
   if cs[b'ISGN']!=original[b'ISGN']or cs[b'OSGN']!=original[b'OSGN']:raise ValueError('Generated shader signature mismatch')
   text+=add(name,data)
  outputs[f'fm5_resolve_{word}_samples.inc']=text.encode()
 report={'schema':1,'passed':True,'scope':'Private user-local generated includes only; not public payload','arrays':records,'array_count':len(records),'input_sha256':inputs,'compiler_sha256':digest(compiler.read_bytes()),'compiler_flags':{'four_samples':'0x8800 strictness plus O3','eight_samples':'0x800 strictness plus default O1'},'generated_shaders_match_existing_gpu_validated_hashes':True,'files':[{'path':name,'bytes':len(data),'sha256':digest(data)}for name,data in outputs.items()]}
 outputs['generation-report.json']=(json.dumps(report,indent=2)+'\n').encode()
 # Commit a complete set only after every input, transformation and compiled
 # result has passed. No source file or original input is ever changed.
 temp=Path(tempfile.mkdtemp(prefix='.fm5-includes-',dir=out.parent))
 try:
  for name,data in outputs.items():(temp/name).write_bytes(data)
  if out.exists():out.rmdir() # Non-recursive: a concurrent new file prevents commit.
  temp.rename(out)
 finally:
  if temp.exists():shutil.rmtree(temp)
 return report

def main():
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--game-exe',required=True,type=Path);host=parser.add_mutually_exclusive_group(required=True)
 host.add_argument('--engine-exe',type=Path,help='Original Azimuth 0.9.9 azimuth.exe')
 host.add_argument('--prepared-host',type=Path,help='Packaged runtime/pc/ForzaMastro5.exe; supplies the same two locale guards')
 parser.add_argument('--output-dir',required=True,type=Path);parser.add_argument('--compiler',type=Path)
 a=parser.parse_args();print(json.dumps(generate(a.game_exe,a.engine_exe,a.output_dir,a.compiler,a.prepared_host),indent=2))
if __name__=='__main__':main()
