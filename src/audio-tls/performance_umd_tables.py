from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import json,re,struct,sys,bisect
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__);import pefile,capstone
p=pefile.PE(str(ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/umd.dll'));raw=p.__data__;base=p.OPTIONAL_HEADER.ImageBase
fs=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in p.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0] for x in fs];cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64)
out=[]
for f in [0x1550,0x276c,0x16fb0]:
    for m in re.finditer(re.escape(struct.pack('<Q',base+f)),raw):
        rva=p.get_rva_from_offset(m.start());buf=p.get_data(rva,640)
        if len(buf)!=640:continue
        ptrs=struct.unpack('<80Q',buf);methods=[]
        for slot in [0,1,2,54,55,56,57,58,59,60,61,62,63]:
            target=ptrs[slot]-base;i=bisect.bisect_right(starts,target)-1
            row={'slot':slot,'pointer':hex(ptrs[slot]),'rva':hex(target)}
            if i>=0 and target<fs[i][1]:
                a,b=fs[i];row['assembly']=[f'{x.address-base:x}: {x.mnemonic} {x.op_str}' for x in cs.disasm(p.get_data(a,b-a),base+a)]
            methods.append(row)
        out.append({'qi':hex(f),'table_rva':hex(rva),'methods':methods})
(ROOT/'analysis/audio-tls/performance-device/umd-device-tables.json').write_text(json.dumps(out,indent=2))
for row in out:
    print('QI',row['qi'],'TABLE',row['table_rva'])
    for m in row['methods']:
        print(m['slot'],m['rva']);
        if m['slot'] in [54,55]:print('\n'.join(m.get('assembly',[])))
