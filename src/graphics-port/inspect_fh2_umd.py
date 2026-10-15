from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import external_fh2_root, project_root
exec((project_root(__file__) / 'src/graphics-port/inspect_standard_qi.py').read_text().split('for mod,funcs')[0])
p=pefile.PE(str(external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'),fast_load=True);p.parse_data_directories(directories=[0,1]);print('base',hex(p.OPTIONAL_HEADER.ImageBase),'machine',hex(p.FILE_HEADER.Machine));print('exports',[(x.name,x.ordinal,hex(x.address))for x in p.DIRECTORY_ENTRY_EXPORT.symbols][:40]);print('import',[(x.dll,len(x.imports))for x in getattr(p,'DIRECTORY_ENTRY_IMPORT',[])])
