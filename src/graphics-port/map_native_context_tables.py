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

paths = [ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll',
         external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll']
guid = uuid.UUID('9458fe06-c78d-47f7-96a0-ec7b727be1e9').bytes_le
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
cs.detail = True
cs.skipdata = True
all_rows = []
for path in paths:
    pe = pefile.PE(str(path), fast_load=True)
    pe.parse_data_directories(directories=[3])
    base = pe.OPTIONAL_HEADER.ImageBase
    code = [s for s in pe.sections if s.Characteristics & 0x20000000]
    ranges = sorted((e.struct.BeginAddress, e.struct.EndAddress) for e in pe.DIRECTORY_ENTRY_EXCEPTION)
    starts = [x[0] for x in ranges]
    addresses = {base + pe.get_rva_from_offset(m.start()) for m in re.finditer(re.escape(guid), pe.__data__)}
    functions = set()
    for section in code:
        for ins in cs.disasm(section.get_data(), base+section.VirtualAddress):
            if ins.id and any(op.type == capstone.CS_OP_MEM and op.mem.base == capstone.x86.X86_REG_RIP
                   and ins.address+ins.size+op.mem.disp in addresses for op in ins.operands):
                index = bisect.bisect_right(starts, ins.address-base)-1
                functions.add(ranges[index][0])
    tables = []
    for function in functions:
        for match in re.finditer(re.escape(struct.pack('<Q', base+function)), pe.__data__):
            table = pe.get_rva_from_offset(match.start())
            entries = []
            for n in range(300):
                data = pe.get_data(table+n*8,8)
                if len(data) != 8: break
                pointer = struct.unpack('<Q',data)[0]
                if not any(base+s.VirtualAddress <= pointer < base+s.VirtualAddress+s.Misc_VirtualSize for s in code): break
                entries.append(hex(pointer-base))
            if len(entries) >= 120:
                tables.append({'rva':hex(table), 'qi':hex(function), 'entries':entries})
    row = {'file':str(path), 'tables':tables}
    all_rows.append(row)
    print(path, [(t['rva'],len(t['entries'])) for t in tables])
(ROOT/'analysis/graphics-port/native-context-tables-comparison.json').write_text(json.dumps(all_rows,indent=2))
