from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_root
import bisect
import json
import re
import struct
import uuid

ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import capstone
import pefile

paths = [ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe',
         external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe']
guid = uuid.UUID('9458fe06-c78d-47f7-96a0-ec7b727be1e9').bytes_le
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
out = []
for path in paths:
    pe = pefile.PE(str(path))
    raw = pe.__data__
    guid_sites = [pe.get_rva_from_offset(m.start()) for m in re.finditer(re.escape(guid), raw)]
    section = next(s for s in pe.sections if s.Name.rstrip(b'\0') == b'.text')
    data = section.get_data()
    table = sorted((x.struct.BeginAddress, x.struct.EndAddress) for x in pe.DIRECTORY_ENTRY_EXCEPTION)
    starts = [x[0] for x in table]
    refs = []
    for match in re.finditer(rb'\xc5\xf8\x10\x05', data):
        k = match.start()
        target = section.VirtualAddress + k + 8 + struct.unpack_from('<i', data, k+4)[0]
        if target not in guid_sites:
            continue
        address = section.VirtualAddress + k
        index = bisect.bisect_right(starts, address) - 1
        begin, end = table[index]
        refs.append({'site': hex(address), 'function': hex(begin),
            'assembly': [f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in
                          cs.disasm(pe.get_data(begin, end-begin), pe.OPTIONAL_HEADER.ImageBase+begin)]})
    out.append({'file': str(path), 'guid_rvas': [hex(x) for x in guid_sites], 'references': refs})
(ROOT / 'analysis/graphics-port/performance-context-comparison.json').write_text(json.dumps(out, indent=2))
for item in out:
    print(item['file'], 'references', len(item['references']))
    for ref in item['references']:
        print('function',ref['function'],'reference',ref['site'])
        for i,line in enumerate(ref['assembly']):
            if 'mov edx, 1' in line:
                print('\n'.join(ref['assembly'][max(0,i-2):i+5]))
