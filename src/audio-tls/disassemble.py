from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile
from capstone import Cs, CS_ARCH_X86, CS_MODE_64
cs = Cs(CS_ARCH_X86, CS_MODE_64)
targets = [
    ('xdk-xaudio-create', ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/xaudio2_9.dll', 0x3c90, 480),
    ('az-tls', ROOT / 'runtime/azimuth-0.9.9-original/azimuth.exe', 0xbe6a0, 480),
    ('az-audio-create', ROOT / 'runtime/azimuth-0.9.9-original/azimuth.exe', 0x181b10, 480),
    ('fm5-audio-start', ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe', 0x10028f0, 1600),
]
for name, path, rva, size in targets:
    pe = pefile.PE(str(path))
    out = '\n'.join(f'{x.address:x}: {x.bytes.hex():24} {x.mnemonic} {x.op_str}' for x in cs.disasm(pe.get_data(rva,size), pe.OPTIONAL_HEADER.ImageBase+rva))
    (ROOT / f'analysis/audio-tls/{name}.asm').write_text(out)
    print(name, out[:6500], sep='\n')
