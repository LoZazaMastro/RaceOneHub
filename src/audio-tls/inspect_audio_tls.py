from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json
import hashlib

ROOT = Path(__file__).resolve().parents[2]
add_python_libraries(__file__)
import pefile

game = ROOT / 'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe'
paths = [game / 'Mount/forza_durango_release_final.exe']
paths += sorted((game / 'EmbeddedXvd/Windows/System32').glob('*audio*'))
paths += sorted((game / 'EmbeddedXvd/Windows/System32').glob('*acp*'))
paths += [ROOT / 'runtime/azimuth-0.9.9-original/azimuth.exe', ROOT / 'runtime/azimuth-0.9.9-original/data/bin/azimuth-tls.dll']
report = []
for path in paths:
    raw = path.read_bytes()
    if raw[:2] != b'MZ':
        continue
    pe = pefile.PE(data=raw)
    tls = None
    if hasattr(pe, 'DIRECTORY_ENTRY_TLS'):
        t = pe.DIRECTORY_ENTRY_TLS.struct
        size = t.EndAddressOfRawData - t.StartAddressOfRawData
        start = t.StartAddressOfRawData - pe.OPTIONAL_HEADER.ImageBase
        template = pe.get_data(start, size)
        tls = {'fields': t.dump(), 'template_hex': template.hex(), 'template_all_zero': not any(template), 'bytes': size,
               'index_rva': hex(t.AddressOfIndex - pe.OPTIONAL_HEADER.ImageBase),
               'callbacks_first_pointer_hex': pe.get_data(t.AddressOfCallBacks-pe.OPTIONAL_HEADER.ImageBase,8).hex() if t.AddressOfCallBacks else None,
               'zero_fill': t.SizeOfZeroFill, 'alignment_field':hex(t.Characteristics)}
    row = {'path': str(path), 'sha256': hashlib.sha256(raw).hexdigest(), 'tls': tls,
           'exports': [{'name': s.name.decode() if s.name else None, 'ordinal': s.ordinal, 'rva': hex(s.address),
                        'forwarder': s.forwarder.decode() if s.forwarder else None}
                       for s in getattr(getattr(pe, 'DIRECTORY_ENTRY_EXPORT', None), 'symbols', [])],
           'imports': [{'dll': d.dll.decode(), 'symbols': [i.name.decode() if i.name else '#' + str(i.ordinal) for i in d.imports]}
                       for d in getattr(pe, 'DIRECTORY_ENTRY_IMPORT', [])],
           'delay_imports': [{'dll': d.dll.decode(), 'symbols': [i.name.decode() if i.name else '#' + str(i.ordinal) for i in d.imports]}
                       for d in getattr(pe, 'DIRECTORY_ENTRY_DELAY_IMPORT', [])]}
    if path.name == 'azimuth.exe':
        row['ascii_audio_strings'] = []
        import re
        for m in re.finditer(rb'[\x20-\x7e]{5,}', raw):
            s = m.group().decode()
            if any(x in s.lower() for x in ['xaudio', 'acphal', 'tls', 'xma']):
                row['ascii_audio_strings'].append({'offset': hex(m.start()), 'rva': hex(pe.get_rva_from_offset(m.start())), 'value': s})
    report.append(row)
(ROOT / 'analysis/audio-tls/pe-inspection.json').write_text(json.dumps(report, indent=2))
print(json.dumps([{'name': Path(x['path']).name, 'tls': x['tls'], 'exports': x['exports'][:20],
                   'imports': x['imports'] if Path(x['path']).name.lower().startswith('xaudio') else [],
                   'strings': x.get('ascii_audio_strings', [])} for x in report], indent=2))

session = ROOT / 'analysis/rea/session'
for n, label, address in [(400, 'tls-bind', '0x1400be6a0')]:
    path = session / f'{n:03d}-audio-tls-{label}.request.json'
    tmp = path.with_suffix('.pending')
    tmp.write_text(json.dumps({'name': 'procedure_pseudo_code', 'arguments': {'procedure': address}}))
    tmp.replace(path)
