from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries, external_fh2_root
import bisect,json,re,struct,sys,uuid
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'analysis/audio-tls/performance-device';OUT.mkdir(exist_ok=True)
add_python_libraries(__file__)
import pefile,capstone
base=ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe'
p=pefile.PE(str(base/'EmbeddedXvd/Windows/System32/d3d11_x.dll'))
guids={e.name.decode():str(uuid.UUID(bytes_le=p.get_data(e.address,16))) for e in p.DIRECTORY_ENTRY_EXPORT.symbols if e.name and b'PerformanceDevice' in e.name}
print('IID exports',guids)
out=[]
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
for label,path in [('FM5',base/'Mount/forza_durango_release_final.exe'),('FH2',external_fh2_root() / 'work/game-complete/Anthem_1.0.0.64_x64__8wekyb3d8bbwe/Mount/forza_durango_release_final.exe'),('Az',ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe')]:
    pe=pefile.PE(str(path));raw=pe.__data__
    funcs=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in pe.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0] for x in funcs]
    for name,guid in guids.items():
        g=uuid.UUID(guid).bytes_le;sites=[pe.get_rva_from_offset(x.start()) for x in re.finditer(re.escape(g),raw)]
        refs=[]
        for s in pe.sections:
            if not s.Characteristics&0x20000000:continue
            d=s.get_data()
            for pat in [rb'\xc5\xf8\x10\x05',rb'\x48\x8d\x15',rb'\x48\x8d\x0d']:
                for m in re.finditer(pat,d):
                    k=m.start();n=len(m.group())
                    if s.VirtualAddress+k+n+4+struct.unpack_from('<i',d,k+n)[0] not in sites:continue
                    site=s.VirtualAddress+k;i=bisect.bisect_right(starts,site)-1
                    if i<0 or site>=funcs[i][1]:continue
                    a,b=funcs[i];refs.append({'reference_rva':hex(site),'procedure_rva':hex(a),'assembly':[f'{x.address-pe.OPTIONAL_HEADER.ImageBase:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(pe.get_data(a,b-a),pe.OPTIONAL_HEADER.ImageBase+a)]})
        out.append({'module':label,'iid_name':name,'guid':guid,'guid_rvas':[hex(x) for x in sites],'references':refs})
(OUT/'iid-and-calls.json').write_text(json.dumps(out,indent=2))
for row in out:
    print(row['module'],row['iid_name'],row['guid'],'refs',[(x['reference_rva'],x['procedure_rva']) for x in row['references']])
