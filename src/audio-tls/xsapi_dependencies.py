from pathlib import Path
import json
ROOT=Path(__file__).resolve().parents[2]
rows=json.loads((ROOT/'analysis/audio-tls/xsapi-pe-comparison.json').read_text())
fm,fh=rows
def imports(r):return {(d['dll'].lower(),s) for d in r['imports'] for s in d['symbols']}
report={'fm5_only_imports':sorted(imports(fm)-imports(fh)), 'fh2_only_imports':sorted(imports(fh)-imports(fm)),
'fm5_import_dlls':[d['dll'] for d in fm['imports']], 'fm5_exports':fm['exports'],
'xsapi_layout_guard':{'function_rva':'0x182800','TLS_directory_size_offset_from_parse_output':'0xec',
'delay_directory_size_offset_from_parse_output':'0x10c','FM5_TLS_size':fm['directories'][9]['bytes'],
'FM5_delay_size':fm['directories'][13]['bytes'],'passed':fm['directories'][9]['bytes']=='0x0' and fm['directories'][13]['bytes']=='0x0'},
'patch_needed':False,'scope':'Static PE and REA dependency/layout analysis; XSAPI DLL not executed.'}
(ROOT/'analysis/audio-tls/xsapi-dependencies.json').write_text(json.dumps(report,indent=2));print(json.dumps(report,indent=2))
