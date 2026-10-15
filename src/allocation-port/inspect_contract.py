from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import sys,struct,json
R=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
m=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
O=R/'analysis/allocation-port'
for label,path,needle,slot in [('umd',R/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',0x19d68,60),('az',R/'runtime/azimuth-0.9.9-original/azimuth.exe',0xc3b20,62)]:
 p=pefile.PE(str(path));b=p.OPTIONAL_HEADER.ImageBase;raw=path.read_bytes(); pat=struct.pack('<Q',b+needle)
 pos=0
 while True:
  pos=raw.find(pat,pos)
  if pos<0:break
  r=p.get_rva_from_offset(pos);print(label,'pointer',hex(r),'possibletable',hex(r-slot*8))
  for n in range(-4,8):
   a=struct.unpack('<Q',p.get_data(r+n*8,8))[0]-b
   print('slot',slot+n,hex(a))
  pos+=1
 if label=='umd':
  # known texture placement entry determines actual vtable.
  r=next(p.get_rva_from_offset(i) for i in range(len(raw)) if raw.startswith(pat,i));tab=r-slot*8
  for n in [58,59,60,61,62,63,64]:
   a=struct.unpack('<Q',p.get_data(tab+n*8,8))[0]-b
   f=next((x.struct for x in p.DIRECTORY_ENTRY_EXCEPTION if x.struct.BeginAddress<=a<x.struct.EndAddress),None)
   sz=f.EndAddress-a if f else 128
   (O/f'umd-slot{n}-{a:x}.txt').write_text('\n'.join(f'{x.address-b:x}: {x.mnemonic} {x.op_str}'for x in m.disasm(p.get_data(a,sz),b+a)))
 for a in ([0x19b78,0x19b8c]if label=='umd'else[0xc3b20,0xf36d0]):
  f=next((x.struct for x in p.DIRECTORY_ENTRY_EXCEPTION if x.struct.BeginAddress<=a<x.struct.EndAddress),None)
  sz=f.EndAddress-a if f else 128
  (O/f'{label}-{a:x}.txt').write_text('\n'.join(f'{x.address-b:x}: {x.mnemonic} {x.op_str}'for x in m.disasm(p.get_data(a,sz),b+a)))
