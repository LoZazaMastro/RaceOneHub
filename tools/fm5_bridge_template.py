"""Strip private FM5 Bridge arrays for distribution; hydrate only in local staging.

Developer: make-template uses one privately built DLL and regenerated includes.
End user: hydrate_bridge needs only pinned game input, packaged prepared host,
pefile and Windows System32 D3DCompiler. It never invokes VS, a shell or a game.
"""
from __future__ import annotations
import argparse
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import struct
import tempfile
import pefile
import generate_fm5_includes as recipe

SCHEMA=1
INCLUDES=('fm5_language_guards.inc','fm5_resolve_original_guard.inc',
          'fm5_resolve_four_samples.inc','fm5_resolve_eight_samples.inc')
HOST_RANGES={'guard_host_locale':(0x37d00,0x37d6e),'guard_host_set_locale':(0x3abf0,0x3ac16)}

def sha(data):return hashlib.sha256(data).hexdigest()
def expected_origins():
 result={name:{'kind':'prepared_host_range','rva':start,'size':end-start}for name,(start,end)in HOST_RANGES.items()}
 result.update({'guard_'+name:{'kind':'game_range','rva':start,'size':end-start}for name,start,end in recipe.LANGUAGE_GUARDS})
 for rva in recipe.SHADERS:
  for tag in ['RDEF','ISGN','OSGN','SHEX']:
   result[f'original_{rva:x}_{tag.lower()}']={'kind':'game_dxbc_chunk','shader_rva':rva,'chunk':tag}
 for count,word in [(4,'four'),(8,'eight')]:
  for tone in [False,True]:
   result['fm5_resolve_'+word+'_samples'+('_tonemap'if tone else'')]={'kind':'system_d3dcompile','samples':count,'tonemap':tone,'debug_shader_rva':0x171a050 if tone else None,'flags':0x8800 if count==4 else 0x800}
 return result
ORIGINS=expected_origins()

def _file(path,label):
 path=Path(path).expanduser()
 if not path.is_absolute()or not path.is_file():raise ValueError(label+' must be an existing absolute file')
 return path.resolve()

def _new_output(path):
 path=Path(path).expanduser()
 if not path.is_absolute():raise ValueError('Output must be absolute')
 if path.exists():raise ValueError('Output already exists; no file is replaced')
 if not path.parent.is_dir():raise ValueError('Output parent must already exist')
 for parent in path.parents:
  if parent.is_symlink()or getattr(parent.lstat(),'st_file_attributes',0)&0x400:raise ValueError('Output cannot traverse a reparse point')
 return path

def _arrays(directory):
 directory=Path(directory)
 report=json.loads((directory/'generation-report.json').read_text(encoding='utf-8'))
 inputs=report.get('input_sha256')
 from_host=isinstance(inputs,dict)and set(inputs)=={'game','prepared_host'}and inputs['game']==recipe.GAME_SHA
 if report.get('array_count')!=28 or inputs!={'game':recipe.GAME_SHA,'engine':recipe.ENGINE_SHA}and not from_host:raise ValueError('Unrecognized include generation provenance')
 records={r['name']:r for r in report['arrays']};values={}
 for filename in INCLUDES:
  text=(directory/filename).read_text(encoding='utf-8')
  for match in re.finditer(r'static const BYTE\s+(\w+)\[\]\s*=\s*\{([^}]+)\}',text):
   name=match[1]
   if name in values or name not in ORIGINS:raise ValueError('Unexpected or duplicate generated array')
   value=bytes(int(x,16)for x in re.findall(r'0x([0-9a-fA-F]{2})',match[2]))
   if not value or records.get(name)!={'name':name,'bytes':len(value),'sha256':sha(value)}:raise ValueError('Generated array integrity mismatch: '+name)
   values[name]=value
 if set(values)!=set(ORIGINS):raise ValueError('Expected all28 private arrays')
 return values

def _data_span(pe,offset,size):
 return any(s.PointerToRawData<=offset and offset+size<=s.PointerToRawData+s.SizeOfRawData and not(s.Characteristics&0x20000000)for s in pe.sections)

def _zero_array_target(raw,pe):
 # This already-zero array cannot be located by zero-byte searching. Its exact
 # compiled reference is the last LanguageGuard record (rva,size,pointer).
 pattern=struct.pack('<I4xQ',0x25d293d,8);matches=[]
 for match in re.finditer(re.escape(pattern),raw):
  if match.start()+24>len(raw):continue
  ptr=struct.unpack_from('<Q',raw,match.start()+16)[0]
  try:offset=pe.get_offset_from_rva(ptr-pe.OPTIONAL_HEADER.ImageBase)
  except(pefile.PEFormatError,ValueError,OverflowError):continue
  if _data_span(pe,offset,8)and raw[offset:offset+8]==bytes(8):matches.append(offset)
 if len(set(matches))!=1:raise ValueError('Cannot uniquely identify zero-valued initial-buffer array')
 return matches[0]

def create_template(bridge_path,includes_directory,prepared_host,output_directory):
 """Developer-only stripping; emits no owner bytes in binary/manifest."""
 bridge_path=_file(bridge_path,'Privately built Bridge');prepared_host=_file(prepared_host,'Prepared host')
 out=_new_output(output_directory);values=_arrays(includes_directory);raw=bridge_path.read_bytes();pe=pefile.PE(data=raw)
 host=pefile.PE(data=prepared_host.read_bytes())
 for name,(start,end)in HOST_RANGES.items():
  if host.get_data(start,end-start)!=values[name]:raise ValueError('Prepared host does not retain the two required original locale guards')
 zeroed=bytearray(raw);records=[];ranges=[]
 for name,value in values.items():
  offsets=[_zero_array_target(raw,pe)]if not any(value)else[m.start()for m in re.finditer(re.escape(value),raw)]
  if not offsets:raise ValueError('Array absent from private Bridge: '+name)
  for offset in offsets:
   if not _data_span(pe,offset,len(value)):raise ValueError('Derived array overlaps executable/non-data bytes: '+name)
   zeroed[offset:offset+len(value)]=bytes(len(value));ranges.append((offset,offset+len(value)))
  records.append({'name':name,'bytes':len(value),'sha256':sha(value),'offsets':offsets,'origin':ORIGINS[name],'already_zero':not any(value)})
 for value in values.values():
  if any(value)and value in zeroed:raise ValueError('Private array occurrence survived stripping')
 ranges.sort();merged=[]
 for start,end in ranges:
  if merged and start<=merged[-1][1]:merged[-1][1]=max(merged[-1][1],end)
  else:merged.append([start,end])
 manifest={'schema':SCHEMA,'kind':'fm5-zero-array-bridge-template','array_count':28,
  'template_bytes':len(raw),'template_sha256':sha(zeroed),'hydrated_sha256':sha(raw),
  'game_sha256':recipe.GAME_SHA,'original_engine_sha256':recipe.ENGINE_SHA,
  'host_guard_source':'The two unchanged code ranges in the packaged prepared host; no separate original host is required.',
  'compiler':'Windows System32/d3dcompiler_47.dll; exact validated shader outputs required',
  'arrays':records,'zero_ranges':[{'offset':a,'bytes':b-a}for a,b in merged],
  'zeroed_bytes':sum(b-a for a,b in merged),'ready_to_load':False,
  'scope':'Only the28 regenerated arrays are stripped. This is not an audit of other engine/licensing dependencies.'}
 temp=Path(tempfile.mkdtemp(prefix='.bridge-template-',dir=out.parent))
 try:
  (temp/'ForzaMastro5Bridge.template').write_bytes(zeroed)
  (temp/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n',encoding='utf-8')
  os.rename(temp,out)
 finally:
  if temp.exists():shutil.rmtree(temp)
 return {'template':str(out/'ForzaMastro5Bridge.template'),'manifest':str(out/'manifest.json'),'arrays':28,'zeroed_bytes':manifest['zeroed_bytes'],'template_sha256':manifest['template_sha256'],'game_executed':False}

def _load_manifest(path,template):
 manifest=json.loads(path.read_text(encoding='utf-8'))
 if manifest.get('schema')!=SCHEMA or manifest.get('kind')!='fm5-zero-array-bridge-template' or manifest.get('array_count')!=28:raise ValueError('Unknown Bridge template manifest')
 if manifest.get('template_bytes')!=len(template)or manifest.get('template_sha256')!=sha(template)or manifest.get('game_sha256')!=recipe.GAME_SHA or manifest.get('original_engine_sha256')!=recipe.ENGINE_SHA:raise ValueError('Bridge template/input identity mismatch')
 records=manifest.get('arrays',[])
 if len(records)!=28 or {x['name']for x in records}!=set(ORIGINS):raise ValueError('Invalid Bridge array set')
 pe=pefile.PE(data=template)
 for record in records:
  if record.get('origin')!=ORIGINS[record['name']]:raise ValueError('Unknown array origin')
  size=record.get('bytes');offsets=record.get('offsets')
  if type(size)is not int or not 0<size<=1048576 or not isinstance(offsets,list)or not 1<=len(offsets)<=128:raise ValueError('Invalid array bounds')
  for offset in offsets:
   if type(offset)is not int or offset<0 or offset+size>len(template)or not _data_span(pe,offset,size):raise ValueError('Array outside read-only/data section')
   if any(template[offset:offset+size]):raise ValueError('Template contains nonzero private array bytes')
 return manifest

def _regenerate(game_exe,prepared_host,manifest):
 _,_,game=recipe.checked_input(game_exe,recipe.GAME_SHA,'FM5 game executable')
 host=pefile.PE(data=prepared_host.read_bytes());guest=game.get_memory_mapped_image();result={}
 for name,(start,end)in HOST_RANGES.items():result[name]=host.get_data(start,end-start)
 for name,start,end in recipe.LANGUAGE_GUARDS:result['guard_'+name]=guest[start:end]
 family={rva:recipe.shader_at(game,rva)for rva in recipe.SHADERS}
 for rva,cs in family.items():
  for tag in [b'RDEF',b'ISGN',b'OSGN',b'SHEX']:result[f'original_{rva:x}_{tag.decode().lower()}']=cs[tag]
 if os.name!='nt':raise RuntimeError('Native Windows System32 D3DCompiler is required')
 directory=C.create_unicode_buffer(32768);kernel=C.WinDLL('kernel32',use_last_error=True)
 get=kernel.GetSystemDirectoryW;get.argtypes=[C.c_wchar_p,C.c_uint];get.restype=C.c_uint
 n=get(directory,len(directory))
 if not 0<n<len(directory):raise RuntimeError('Cannot resolve the actual Windows System32 directory')
 compiler=Path(directory.value)/'d3dcompiler_47.dll'
 if not compiler.is_file():raise RuntimeError('Windows System32/d3dcompiler_47.dll is unavailable')
 streams=recipe.msf_streams(family[0x171a050][b'SPDB'])
 for count,word in [(4,'four'),(8,'eight')]:
  for tone in [False,True]:
   name='fm5_resolve_'+word+'_samples'+('_tonemap'if tone else'')
   source=recipe.transformed_tonemap(streams,count)if tone else recipe.authored_identity(count)
   code=recipe.compile_shader(source,compiler,0x8800 if count==4 else 0x800)
   size,expected=recipe.EXPECTED_COMPILED[name]
   if len(code)!=size or sha(code)!=expected:raise ValueError('System shader compiler output differs from validated baseline: '+name)
   original=family[0x171a050 if tone else 0x11d41d0];cs=recipe.chunks(code)
   if cs[b'ISGN']!=original[b'ISGN']or cs[b'OSGN']!=original[b'OSGN']:raise ValueError('Compiled shader signatures do not match selected game')
   result[name]=code
 return result

def _validate_recovered_arrays(values,manifest):
 if set(values)!=set(ORIGINS):raise ValueError('Recovered array set differs')
 for item in manifest['arrays']:
  value=values[item['name']]
  if len(value)!=item['bytes']or sha(value)!=item['sha256']:raise ValueError('Locally recovered array differs: '+item['name'])

def hydrate_bridge(template_path,manifest_path,game_exe,prepared_host,output_bridge):
 """Recover28 private arrays in a NEW local output. No compiler SDK/VS needed."""
 template_path=_file(template_path,'Bridge template');manifest_path=_file(manifest_path,'Bridge manifest');game_exe=_file(game_exe,'FM5 game executable');prepared_host=_file(prepared_host,'Prepared host');out=_new_output(output_bridge)
 game_root=game_exe.parent.parent if game_exe.parent.name.casefold()=='mount'else game_exe.parent
 if out==game_root or game_root in out.parents:raise ValueError('Output cannot modify the original game folder')
 raw=template_path.read_bytes();manifest=_load_manifest(manifest_path,raw);values=_regenerate(game_exe,prepared_host,manifest)
 _validate_recovered_arrays(values,manifest)
 hydrated=bytearray(raw);written={}
 for item in manifest['arrays']:
  value=values[item['name']]
  for offset in item['offsets']:
   for relative,byte in enumerate(value):
    at=offset+relative
    if at in written and written[at]!=byte:raise ValueError('Overlapping array recovery disagrees')
    written[at]=byte
   hydrated[offset:offset+len(value)]=value
 if sha(hydrated)!=manifest['hydrated_sha256']:raise ValueError('Recovered Bridge differs from the developer baseline')
 fd,tempname=tempfile.mkstemp(prefix='.bridge-hydrate-',suffix='.tmp',dir=out.parent);temp=Path(tempname)
 try:
  with os.fdopen(fd,'wb')as stream:stream.write(hydrated);stream.flush();os.fsync(stream.fileno())
  os.rename(temp,out) # Windows rename refuses an existing destination, including races.
 finally:
  if temp.exists():temp.unlink()
 return {'bridge':str(out),'sha256':manifest['hydrated_sha256'],'arrays':28,'compiler':'Windows System32/d3dcompiler_47.dll','game_executed':False,'host_executed':False}

def main():
 parser=argparse.ArgumentParser(description=__doc__);commands=parser.add_subparsers(dest='command',required=True)
 make=commands.add_parser('make-template');make.add_argument('--bridge',required=True,type=Path);make.add_argument('--includes-dir',required=True,type=Path);make.add_argument('--prepared-host',required=True,type=Path);make.add_argument('--output-dir',required=True,type=Path)
 use=commands.add_parser('hydrate');use.add_argument('--template',required=True,type=Path);use.add_argument('--manifest',required=True,type=Path);use.add_argument('--game-exe',required=True,type=Path);use.add_argument('--prepared-host',required=True,type=Path);use.add_argument('--output-bridge',required=True,type=Path)
 args=parser.parse_args()
 if args.command=='make-template':result=create_template(args.bridge,args.includes_dir,args.prepared_host,args.output_dir)
 else:result=hydrate_bridge(args.template,args.manifest,args.game_exe,args.prepared_host,args.output_bridge)
 print(json.dumps(result,indent=2))
if __name__=='__main__':main()
