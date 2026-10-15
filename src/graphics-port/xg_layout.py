from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_root
import json

ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import capstone
import pefile

files = [
    ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/xg_x.dll',
    external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/xg_x.dll',
]
rows = []
for path in files:
    pe = pefile.PE(str(path))
    rows.append({
        'path': str(path), 'sections': pe.FILE_HEADER.NumberOfSections,
        'image_size': hex(pe.OPTIONAL_HEADER.SizeOfImage),
        'entry_rva': hex(pe.OPTIONAL_HEADER.AddressOfEntryPoint),
        'directories': [dict(index=i, rva=hex(d.VirtualAddress), bytes=d.Size)
                        for i,d in enumerate(pe.OPTIONAL_HEADER.DATA_DIRECTORY)],
    })
az = pefile.PE(str(ROOT / 'runtime/azimuth-0.9.9-original/azimuth.exe'))
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
assembly = [f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in
            cs.disasm(az.get_data(0x9edf0, 0xe50), 0x14009edf0)]
(ROOT / 'analysis/graphics-port/xg-layout-comparison.json').write_text(json.dumps(rows, indent=2))
(ROOT / 'analysis/graphics-port/xg-mapper-assembly.txt').write_text('\n'.join(assembly))
print(json.dumps(rows, indent=2))
for index, line in enumerate(assembly):
    if any(x in line for x in ['0x1d000', '0xdf84', 'word ptr [rsp + 0x17e]', ', 6']):
        print('\n'.join(assembly[max(0,index-2):index+3]))
