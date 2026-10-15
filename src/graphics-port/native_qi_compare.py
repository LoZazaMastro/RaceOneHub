from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import external_fh2_root, project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0])
for label,path,fs in [('FM5',r/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',[0x6ae8,0xb150]),('FH2',external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',[0x8c20,0xfdc0])]:
 p=pefile.PE(str(path),fast_load=True);p.parse_data_directories(directories=[3]);rs=sorted((e.struct.BeginAddress,e.struct.EndAddress)for e in p.DIRECTORY_ENTRY_EXCEPTION);st=[a for a,z in rs]
 for f in fs:
  a,z=rs[bisect.bisect_right(st,f)-1];print(label,hex(f));print('\n'.join(f'{i.address-0x180000000:x}: {i.mnemonic} {i.op_str}'for i in cs.disasm(p.get_data(a,z-a),0x180000000+a)))
