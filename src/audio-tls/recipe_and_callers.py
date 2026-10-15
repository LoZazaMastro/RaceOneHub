from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
from collections import deque
import hashlib
import json
import struct
ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
from capstone.x86 import X86_OP_MEM, X86_REG_RIP
OUT = ROOT / 'analysis/audio-tls'
az = ROOT / 'runtime/azimuth-0.9.9-original/azimuth.exe'
raw = az.read_bytes()
assert hashlib.sha256(raw).hexdigest() == '579ed8d8b0b16cbf7890380cb7b925c972b837631ede367eada55eae4afc39bc'
pe = pefile.PE(data=raw)
patches = []
for rva, before, after, why in [
    (0xbe6de, '50010000', '28000000', 'Require exact FM5 forty-byte TLS template'),
    (0xbe6f5, '00005000', '00004000', 'Require exact FM5 PE TLS alignment8'),
    (0xbe710, '50010000', '28000000', 'Validate precisely all forty FM5 TLS template bytes'),
]:
    offset = pe.get_offset_from_rva(rva)
    assert raw[offset:offset+4].hex() == before
    patches.append({'rva': rva, 'file_offset': offset, 'before_hex': before, 'after_hex': after, 'reason': why})
(OUT / 'tls-patch-recipe.json').write_text(json.dumps({'source_sha256': hashlib.sha256(raw).hexdigest(),
    'fm5_only': True, 'patches': patches,
    'unchanged': ['TLS callback emptiness guard','zero-fill guard','template zero check','image bounds check',
                  'helper native module initialization guard','helper export validation','Windows static TLS allocation'],
    'precondition': 'Exact supplied FM5 build, no TLS callbacks, official 336-byte zero helper retained'}, indent=2))
game = ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'
gp = pefile.PE(str(game))
iat = next(i.address for d in gp.DIRECTORY_ENTRY_IMPORT if d.dll.lower()==b'xaudio2_9.dll' for i in d.imports if i.ordinal==1)
acp_iat = next(i.address for d in gp.DIRECTORY_ENTRY_IMPORT if d.dll.lower()==b'acphal.dll' for i in d.imports if i.name==b'AcpHalAllocateShapeContexts')
cs = Cs(CS_ARCH_X86, CS_MODE_64)
cs.detail = True
matches = []
acp_matches = []
raw_acp = []
for section in gp.sections:
    if not section.Characteristics & 0x20000000: continue
    data=section.get_data()
    for opcode in [b'\xff\x15',b'\xff\x25',b'\x48\x8b\x05']:
        pos=0
        while (pos:=data.find(opcode,pos))>=0:
            end=pos+len(opcode)+4
            if end<=len(data):
                address=gp.OPTIONAL_HEADER.ImageBase+section.VirtualAddress+pos
                if address+len(opcode)+4+struct.unpack_from('<i',data,pos+len(opcode))[0]==acp_iat:
                    start=max(0,pos-160)
                    raw_acp.append({'rva':hex(address-gp.OPTIONAL_HEADER.ImageBase),
                        'before_hex':data[start:end].hex(), 'start_rva':hex(section.VirtualAddress+start)})
            pos+=1
    previous = deque(maxlen=35)
    for ins in cs.disasm(section.get_data(), gp.OPTIONAL_HEADER.ImageBase + section.VirtualAddress):
        line = f'{ins.address:x}: {ins.bytes.hex():24} {ins.mnemonic} {ins.op_str}'
        for operand in ins.operands:
            if operand.type != X86_OP_MEM: continue
            mem = operand.mem
            if mem.base == X86_REG_RIP and ins.address+ins.size+mem.disp == iat:
                matches.append({'call_rva': hex(ins.address-gp.OPTIONAL_HEADER.ImageBase), 'assembly': list(previous)+[line]})
            if mem.base == X86_REG_RIP and ins.address+ins.size+mem.disp == acp_iat:
                acp_matches.append({'call_rva': hex(ins.address-gp.OPTIONAL_HEADER.ImageBase), 'assembly': list(previous)+[line]})
        previous.append(line)
(OUT / 'fm5-xaudio-callers.json').write_text(json.dumps({'iat_va':hex(iat), 'matches':matches,
    'limitation':'Capstone linear disassembly can miss code after invalid or embedded bytes; static call-site evidence only.'}, indent=2))
print(json.dumps(matches,indent=2))
(OUT / 'fm5-acp-callers.json').write_text(json.dumps({'iat_va':hex(acp_iat),'matches':acp_matches,'raw_direct_candidates':raw_acp},indent=2))
print(json.dumps(acp_matches,indent=2))
print(json.dumps(raw_acp,indent=2))
