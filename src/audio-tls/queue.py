from pathlib import Path
import json
ROOT = Path(__file__).resolve().parents[2]
S = ROOT / 'analysis/rea/session'
for n, label, tool, key, target in [
    (401, 'audio-create-xrefs', 'xrefs', 'address', '0x1402b4e00'),
    (402, 'audio-dll-xrefs', 'xrefs', 'address', '0x1402a6278'),
    (403, 'acphal-xrefs', 'xrefs', 'address', '0x14003a877'),
    (404, 'audio-create', 'procedure_pseudo_code', 'procedure', '0x140181b9e'),
    (405, 'audio-import', 'procedure_pseudo_code', 'procedure', '0x14009a15a'),
    (409, 'acp-allocate-xrefs', 'xrefs', 'address', '0x1402a7840'),
    (410, 'acp-name-xrefs', 'xrefs', 'address', '0x1402a7870'),
    (411, 'acp-allocate', 'procedure_pseudo_code', 'procedure', '0x1400b0e00'),
    (412, 'acp-import-map', 'procedure_pseudo_code', 'procedure', '0x14003a844'),
    (413, 'xsapi-pe-parse', 'procedure_pseudo_code', 'procedure', '0x14009b9c0'),
    (414, 'xsapi-init', 'procedure_pseudo_code', 'procedure', '0x140182800'),
    (415, 'crt-resolver', 'procedure_pseudo_code', 'procedure', '0x140039bc0'),
    (416, 'import-resolver', 'procedure_pseudo_code', 'procedure', '0x14003a290'),
    (417, 'device-slot37', 'procedure_pseudo_code', 'procedure', '0x1400589b0'),
    (418, 'device-slot36-default', 'procedure_pseudo_code', 'procedure', '0x1400f9530'),
    (419, 'device-constructor', 'procedure_pseudo_code', 'procedure', '0x1400daae0'),
    (420, 'device-slot3e', 'procedure_pseudo_code', 'procedure', '0x140121870'),
    (421, 'device-slot40', 'procedure_pseudo_code', 'procedure', '0x140121c40'),
    (422, 'device-slot41', 'procedure_pseudo_code', 'procedure', '0x140122090'),
    (423, 'device-slot3c', 'procedure_pseudo_code', 'procedure', '0x1400c39f0'),
    (424, 'device-slot3d', 'procedure_pseudo_code', 'procedure', '0x1400c3a80'),
]:
    p = S / f'{n}-audio-tls-{label}.request.json'
    if not p.exists():
        t = p.with_suffix('.pending')
        t.write_text(json.dumps({'name': tool, 'arguments': {key: target}}))
        t.replace(p)
for p in S.glob('*-audio-tls-*.response.json'):
    v = json.loads(p.read_text()).get('result', {}).get('structuredContent', {}).get('raw_result')
    out = ROOT / 'analysis/audio-tls' / p.name.replace('.response.json', '')
    if isinstance(v, dict) and isinstance(v.get('value'), str):
        out.with_suffix('.c').write_text(v['value'])
    else:
        out.with_suffix('.json').write_text(json.dumps(v, indent=2))
