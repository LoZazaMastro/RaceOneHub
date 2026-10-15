"""Package an explicit support-file map for the authored single-EXE container.

This tool does not select or download any engine/game inputs. The caller owns
the provenance review of every explicitly named component.
"""
from pathlib import Path, PurePosixPath
import argparse
import hashlib
import json
import os
import re
import stat
import tempfile
import zipfile

ORIGINS = {'authored', 'license', 'authorized-ui-artwork', 'game-free-template', 'external-engine'}
MAXIMUM_BYTES = 512*1024*1024


def relative_name(value):
    parts = value.split('/') if isinstance(value,str) else []
    if not parts or any(p in ('','.','..') or p.endswith(('.', ' ')) or re.fullmatch(r'CON|PRN|AUX|NUL|COM[1-9]|LPT[1-9]',p.split('.')[0],re.I) for p in parts) or any(c in value for c in '\\:\0'):
        raise ValueError('Invalid embedded support path.')
    return str(PurePosixPath(value))


def package(mapping, output):
    if not isinstance(mapping,list) or not 1 <= len(mapping) <= 512:
        raise ValueError('Supply an explicit list of 1 to 512 support files.')
    output=Path(output).absolute()
    if output.exists():
        raise ValueError('Existing support archives are not replaced.')
    records=[]; sources=[]; names=set(); total=0
    for item in mapping:
        if not isinstance(item,dict) or set(item) != {'path','source','origin'} or item['origin'] not in ORIGINS:
            raise ValueError('Each component needs an explicit path, source and provenance category.')
        name=relative_name(item['path'])
        if name.casefold() in names or name.casefold() == 'manifest.json':
            raise ValueError('Duplicate or reserved support entry.')
        names.add(name.casefold())
        source=Path(item['source']).absolute()
        for ancestor in (source,*source.parents):
            info=ancestor.lstat()
            if stat.S_ISLNK(info.st_mode) or getattr(info,'st_file_attributes',0)&0x400:
                raise ValueError('Support source must not use links or reparse points.')
        if not source.is_file():
            raise ValueError('Support source must be a regular file.')
        raw=source.read_bytes(); total+=len(raw)
        if total>MAXIMUM_BYTES:
            raise ValueError('Support payload exceeds the extraction limit.')
        records.append({'path':name,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest(),'origin':item['origin']})
        sources.append((name,raw))
    manifest={'schema':1,'files':sorted(records,key=lambda r:r['path'])}
    output.parent.mkdir(parents=True,exist_ok=True)
    descriptor, temporary=tempfile.mkstemp(prefix='.raceone-support-',suffix='.zip',dir=output.parent)
    os.close(descriptor)
    try:
        with zipfile.ZipFile(temporary,'w',compression=zipfile.ZIP_DEFLATED,compresslevel=9) as archive:
            entries=[('manifest.json',(json.dumps(manifest,indent=2)+'\n').encode()),*sorted(sources)]
            for name,raw in entries:
                info=zipfile.ZipInfo(name,date_time=(1980,1,1,0,0,0))
                info.compress_type=zipfile.ZIP_DEFLATED
                info.external_attr=(stat.S_IFREG|0o600)<<16
                archive.writestr(info,raw)
        with zipfile.ZipFile(temporary) as archive:
            if archive.testzip() is not None:
                raise RuntimeError('Embedded archive integrity check failed.')
        # No replace: a competing process may have created the destination.
        os.rename(temporary,output)
    finally:
        Path(temporary).unlink(missing_ok=True)
    return {'archive':str(output),'sha256':hashlib.sha256(output.read_bytes()).hexdigest(),
            'files':len(records),'uncompressed_bytes':total,'manifest':manifest}


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--map',type=Path,required=True)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--report',type=Path)
    args=parser.parse_args()
    result=package(json.loads(args.map.read_text('utf-8')),args.output)
    if args.report:
        args.report.write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({k:v for k,v in result.items() if k!='manifest'},indent=2))
