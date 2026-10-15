from pathlib import Path
import sys
sys.path.insert(0, str(next(p / 'tools' for p in Path(__file__).resolve().parents if (p / 'tools/project_paths.py').is_file())))
from project_paths import add_python_libraries
import bisect,json,sys
ROOT=Path(__file__).resolve().parents[2];add_python_libraries(__file__)
import pefile,capstone
OUT=ROOT/'analysis/audio-tls/xg-port';OUT.mkdir(exist_ok=True)
cs=capstone.Cs(capstone.CS_ARCH_X86,capstone.CS_MODE_64);cs.detail=True
sources=[('old',ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32/xg.dll'),('az',ROOT/'runtime/azimuth-0.9.9-original/azimuth.exe')]
results={}
for label,path in sources:
    pe=pefile.PE(str(path));base=pe.OPTIONAL_HEADER.ImageBase
    functions=sorted((x.struct.BeginAddress,x.struct.EndAddress) for x in pe.DIRECTORY_ENTRY_EXCEPTION);starts=[x[0] for x in functions]
    exports={x.name.decode():x.address for x in getattr(pe,'DIRECTORY_ENTRY_EXPORT',[]).symbols} if hasattr(pe,'DIRECTORY_ENTRY_EXPORT') else {}
    ranges=([(0x2a70,0x2b60),(0x2b60,0x2c50),(0x2c50,0x2d40),(0x1460,0x15c0),(0x15c0,0x1fa0),(0x1fa0,0x20f0),(0x13d0,0x1460)] if label=='old' else [(0x9f890,0x9fa30),(0x9fa30,0x9fba0),(0x9fba0,0x9fec0),(0xa3570,0xa37c0)])
    procedures=[]
    for a,b in ranges:
        target=a
        assembly=[]
        for ins in cs.disasm(pe.get_data(a,b-a),base+a):
            assembly.append(f'{ins.address-base:x}: {ins.mnemonic} {ins.op_str}')
        procedures.append({'requested':hex(target),'begin':hex(a),'end':hex(b),'fingerprint16':pe.get_data(target,16).hex(),'assembly':assembly})
    results[label]={'path':str(path),'exports':{k:hex(v) for k,v in exports.items()},'procedures':procedures}
(OUT/'input-initializers.json').write_text(json.dumps(results,indent=2))
for label,result in results.items():
    print(label,result['exports'])
    for item in result['procedures']:print(item['requested'],item['begin'],'\n'+'\n'.join(item['assembly']))
