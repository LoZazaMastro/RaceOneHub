from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_root, project_root
import sys,json
ROOT=project_root(__file__)
add_python_libraries(__file__)
import pefile,capstone
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
files=list((ROOT/'game').rglob('d3d11_x.dll'))
files+=list((external_fh2_root()).glob('work/**/d3d11_x.dll'))[:2]
files+=list((external_fh2_root()).glob('work/game/**/d3d11_x.dll'))[:1]
files+=[ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe']
out=[]
for path in files:
 pe=pefile.PE(str(path));out.append({'file':str(path),'base':hex(pe.OPTIONAL_HEADER.ImageBase),'exports':[]})
 if hasattr(pe,'DIRECTORY_ENTRY_EXPORT'):
  for e in pe.DIRECTORY_ENTRY_EXPORT.symbols:
   asm=[f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in cs.disasm(pe.get_data(e.address,256),pe.OPTIONAL_HEADER.ImageBase+e.address)]
   out[-1]['exports'].append({'name':e.name.decode() if e.name else None,'rva':hex(e.address),'ordinal':e.ordinal,'assembly':asm})
 if path.suffix=='.exe':
  out[-1]['factory']=[f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in cs.disasm(pe.get_data(0x58090,320),pe.OPTIONAL_HEADER.ImageBase+0x58090)]
(ROOT/'analysis/graphics-port/export-assembly.json').write_text(json.dumps(out,indent=2))
print(json.dumps(out,indent=2))
