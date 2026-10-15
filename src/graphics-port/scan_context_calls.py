from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json
import re
import struct

ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import capstone
import pefile

pe = pefile.PE(str(ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'))
section = next(x for x in pe.sections if x.Name.rstrip(b'\0') == b'.text')
data = section.get_data()
base = pe.OPTIONAL_HEADER.ImageBase + section.VirtualAddress
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
out = []
for slot in [0x9a, 0x9b, 0xd6, 0x1b, 0x1c, 0x1d, 0x6f]:
    displacement = slot * 8
    pattern = rb'\xff[\x90-\x97]' + re.escape(struct.pack('<I', displacement))
    if displacement < 128:
        pattern += rb'|\xff[\x50-\x57]' + re.escape(bytes([displacement]))
    group = []
    for m in re.finditer(pattern, data):
        k = m.start()
        start = k-80
        asm = [f'{i.address:x}: {i.mnemonic} {i.op_str}'
               for i in cs.disasm(data[start:k+60], base+start)]
        group.append({'rva': hex(section.VirtualAddress + k), 'assembly': asm})
    out.append({'slot': hex(slot), 'byte_offset': hex(displacement), 'sites': group})
    print('slot', hex(slot), 'sites', len(group), [x['rva'] for x in group])
(ROOT / 'analysis/graphics-port/fm5-context-call-candidates.json').write_text(json.dumps(out, indent=2))
