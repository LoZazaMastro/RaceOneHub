from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json

ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import capstone
import pefile

pe = pefile.PE(str(ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'))
section = next(x for x in pe.sections if x.Name.rstrip(b'\0') == b'.text')
data = section.get_data()
base = pe.OPTIONAL_HEADER.ImageBase + section.VirtualAddress
cs = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_64)
cs.detail = True
out = []
for index in range(96, len(data) - 2):
    if not 0x70 <= data[index] <= 0x7f:
        continue
    rel = data[index + 1] - 256
    if rel >= -2 or rel < -96:
        continue
    start = index + 2 + rel
    instructions = list(cs.disasm(data[start:index+2], base+start))
    if not instructions or instructions[-1].address != base+index:
        continue
    memory_loads = [i for i in instructions if i.mnemonic == 'mov' and len(i.operands) == 2
                    and i.operands[0].type == capstone.CS_OP_REG and i.operands[0].size == 4
                    and i.operands[1].type == capstone.CS_OP_MEM]
    compares = [i for i in instructions if i.mnemonic in ['sub', 'cmp', 'test']
                and i.operands and i.operands[0].size == 4]
    memory_compare = [i for i in compares if any(o.type == capstone.CS_OP_MEM for o in i.operands)]
    if (not memory_loads and not memory_compare) or not compares:
        continue
    out.append({'rva': hex(base + start - pe.OPTIONAL_HEADER.ImageBase),
                'branch_rva': hex(base + index - pe.OPTIONAL_HEADER.ImageBase),
                'assembly': [f'{i.address:x}: {i.mnemonic} {i.op_str}' for i in instructions]})
(ROOT / 'analysis/graphics-port/memory-counter-loop-candidates.json').write_text(json.dumps(out, indent=2))
print('Candidates:', len(out))
