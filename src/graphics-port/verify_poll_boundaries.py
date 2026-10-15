from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect
import json

ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import capstone
import pefile

pe = pefile.PE(str(ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'))
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
ranges = sorted((x.struct.BeginAddress, x.struct.EndAddress) for x in pe.DIRECTORY_ENTRY_EXCEPTION)
starts = [x[0] for x in ranges]
out = []
for candidate in json.loads((ROOT / 'analysis/graphics-port/pure-poll-candidates.json').read_text()):
    start = int(candidate['rva'], 16)
    branch = int(candidate['branch_rva'], 16)
    index = bisect.bisect_right(starts, branch) - 1
    first, end = ranges[index]
    if not first <= start < branch < end:
        continue
    instructions = list(cs.disasm(pe.get_data(first, end - first), pe.OPTIONAL_HEADER.ImageBase + first))
    by_address = {i.address: i for i in instructions}
    if pe.OPTIONAL_HEADER.ImageBase + start not in by_address or pe.OPTIONAL_HEADER.ImageBase + branch not in by_address:
        continue
    candidate['function_rva'] = hex(first)
    candidate['function_assembly'] = [f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in instructions]
    out.append(candidate)
(ROOT / 'analysis/graphics-port/pdata-validated-poll-candidates.json').write_text(json.dumps(out, indent=2))
print(json.dumps(out, indent=2))
