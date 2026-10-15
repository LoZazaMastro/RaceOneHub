"""Headless differential test: authored proxy + FM5 originals, modern local oracle.

Neither modern DLL nor original code/data is copied into any source or module.
No game, engine, UI, or GPU process is started.
"""
from pathlib import Path
import ctypes as C
import hashlib,json,subprocess,sys,time
ROOT=Path(__file__).resolve().parents[1]
OUT=ROOT/'analysis/public-runtime-preparation/portable-xg'
VC=Path(r'C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Tools\MSVC\14.44.35207')
SDK=Path(r'C:\Program Files (x86)\Windows Kits\10')
VER='10.0.26100.0'
P=C.c_void_p;U=C.c_uint32;Q=C.c_uint64;H=C.c_int32
REF=C.WINFUNCTYPE(U,P);LAY=C.WINFUNCTYPE(H,P,P)
COPY=C.WINFUNCTYPE(H,P,P,U,U,P,U,U)
MIP=C.WINFUNCTYPE(Q,P,U,U)
ELEMENT=C.WINFUNCTYPE(Q,P,U,U,Q,U,U,U)
TILING=C.WINFUNCTYPE(H,P,P,P,P,P,U,P)
checks=0
def check(ok,label):
    global checks
    if not ok:raise AssertionError(label)
    checks+=1
def table(p):return C.cast(p,C.POINTER(C.POINTER(P))).contents
def create(module,dimension,desc):
    fn=getattr(module,f'XGCreateTexture{dimension}DComputer');fn.argtypes=[P,C.POINTER(P)];fn.restype=H
    p=P();hr=fn(desc,C.byref(p));return hr,p.value,C.cast(fn,P).value
def describe(dimension,fmt,width,height,depth,mips,arrays,mode,samples=1,quality=0,bind=8,misc=0):
    d=(U*15)()
    if dimension==1:
        d[0],d[1],d[2],d[3],d[5],d[7],d[10]=width,mips,arrays,fmt,bind,misc,mode
    elif dimension==2:
        d[0],d[1],d[2],d[3],d[4],d[5],d[6],d[8],d[10],d[13]=width,height,mips,arrays,fmt,samples,quality,bind,misc,mode
    else:
        d[0],d[1],d[2],d[3],d[4],d[6],d[8],d[11]=width,height,depth,mips,fmt,bind,misc,mode
    return d
def run():
    OUT.mkdir(parents=True,exist_ok=True)
    dll=OUT/'portable-xg.dll';src=ROOT/'src/fm5_portable_xg.c'
    cmd=[str(VC/'bin/Hostx64/x64/cl.exe'),'/nologo','/W4','/WX','/O2','/MT','/LD','/I'+str(VC/'include')]
    cmd+=['/I'+str(SDK/'Include'/VER/x)for x in ['ucrt','shared','um']]
    cmd+=[str(src),'/Fo'+str(OUT)+'\\','/Fe'+str(dll),'/link','/Brepro','/LIBPATH:'+str(VC/'lib/x64')]
    cmd+=['/LIBPATH:'+str(SDK/'Lib'/VER/x/'x64')for x in ['ucrt','um']]
    cmd+=['kernel32.lib','bcrypt.lib']
    result=subprocess.run(cmd,cwd=OUT,text=True,capture_output=True)
    (OUT/'compile.log').write_text(result.stdout+result.stderr)
    check(result.returncode==0,result.stdout+result.stderr)
    # Explicit original inputs; shim is authored project code.
    game=ROOT/'game/Forza_1.0.0.20_x64__8wekyb3d8bbwe/EmbeddedXvd/Windows/System32'
    kernel=C.WinDLL(str(ROOT/'runtime/xg-host/kernelx.dll'))
    oracle_path=ROOT/'analysis/xg-modern/xg_x.dll'
    check(hashlib.sha256(oracle_path.read_bytes()).hexdigest()=='9e3a06d02784699c252bea271090b9e2a04ef2cb29b5b0647c6ce6e73b19db67','local oracle hash')
    modern=C.WinDLL(str(oracle_path));api=C.WinDLL(str(dll))
    make=api.FM5PortableCreateHostTextureComputer;make.argtypes=[P,C.POINTER(P),P];make.restype=H
    out=P(123);check(make(None,C.byref(out),None)<0 and not out.value,'null descriptor')
    check(make(None,None,None)<0,'null output')
    d=describe(2,28,64,64,1,1,1,14)
    @C.WINFUNCTYPE(H,P,C.POINTER(P))
    def wrong(a,b):raise AssertionError('unverified factory called')
    check(make(d,C.byref(out),C.cast(wrong,P))<0 and not out.value,'unknown original factory refused')
    rows=[];msaa_rows=[];real_profiles=[];npot_checks=0;started=time.perf_counter()
    for name in ['xg.dll','xg_x.dll']:
        original=C.WinDLL(str(game/name))
        for dim in [1,2,3]:
            for fmt,block,bytes_ in [(28,1,4),(71,4,8),(77,4,16),(80,4,8),(98,4,16),(99,4,16)]:
                for mode in [8,13,14]:
                    # Linear layout old/new support may differ; test only genuine
                    # successes, and record all creation errors for review.
                    arrays=2 if dim in [1,2] else 1
                    mips=4;depth=5 if dim==3 else 1
                    desc=describe(dim,fmt,19,13,depth,mips,arrays,mode)
                    fn=getattr(original,f'XGCreateTexture{dim}DComputer');fn.argtypes=[P,C.POINTER(P)];fn.restype=H
                    p=P();r=make(desc,C.byref(p),C.cast(fn,P));mhr,mp,_=create(modern,dim,desc)
                    if mhr<0:
                        check(r==mhr,'matching modern creation refusal')
                        if r>=0 and p.value:REF(table(p)[1])(p)
                        rows.append({'library':name,'dim':dim,'format':fmt,'mode':mode,'hr':r,'oracle_hr':mhr,'comparison':'oracle rejects this profile; no equivalence claim'});continue
                    check(r==mhr,(name,dim,fmt,mode,'creation HR',r,mhr))
                    if r<0:rows.append({'library':name,'dim':dim,'format':fmt,'mode':mode,'hr':r});continue
                    check(p.value and mp,'both computers owned')
                    t,mt=table(p),table(mp)
                    a=C.create_string_buffer(b'\xa5'*(0xd40+32));b=C.create_string_buffer(b'\xa5'*(0xd40+32))
                    check(LAY(t[2])(p,a)==LAY(mt[2])(mp,b)==0,'layout HRESULT')
                    check(a.raw==b.raw,(name,dim,fmt,mode,'exact compact layout/canaries',[(hex(i),a.raw[i:i+4].hex(),b.raw[i:i+4].hex())for i in range(0,0xd40,4)if a.raw[i:i+4]!=b.raw[i:i+4]][:24]))
                    resource=int.from_bytes(a.raw[:8],'little')
                    check(REF(t[0])(p)==2 and REF(t[1])(p)==1,'proxy reference ownership')
                    # Mip/element forwarding contract, including invalid sentinel.
                    check(MIP(t[3])(p,0,0)==MIP(mt[3])(mp,0,0),'mip offset')
                    check(ELEMENT(t[4])(p,0,0,0,0,0,0)==ELEMENT(mt[4])(mp,0,0,0,0,0,0),'element offset')
                    for sub in range(mips*arrays):
                        level=sub%mips;w=max(1,19>>level);h=1 if dim==1 else max(1,13>>level);z=max(1,depth>>level)
                        pitch=((w+block-1)//block)*bytes_+16;count=(h+block-1)//block;slice_=pitch*count+32;extent=slice_*z
                        payload=bytearray(b'\xcc'*(extent+32));row_bytes=((w+block-1)//block)*bytes_
                        for k in range(z):
                            for y in range(count):
                                at=k*slice_+y*pitch;payload[at:at+row_bytes]=bytes((i*17+y*13+k*29+sub*23)&255 for i in range(row_bytes))
                        source=C.create_string_buffer(bytes(payload),len(payload))
                        tiled=C.create_string_buffer(b'\x7a'*(resource+32),resource+32)
                        tiled_ref=C.create_string_buffer(b'\x7a'*(resource+32),resource+32)
                        check(COPY(t[5])(p,tiled,0,sub,source,pitch,slice_)==COPY(mt[5])(mp,tiled_ref,0,sub,source,pitch,slice_)==0,'genuine CopyInto')
                        check(tiled.raw==tiled_ref.raw,'identical original/modern tiled bytes + canary')
                        dest=C.create_string_buffer(b'\xa5'*(extent+32),extent+32)
                        ref=C.create_string_buffer(b'\xa5'*(extent+32),extent+32)
                        check(COPY(t[6])(p,dest,0,sub,tiled,pitch,slice_)==COPY(mt[6])(mp,ref,0,sub,tiled_ref,pitch,slice_)==0,(name,dim,fmt,mode,sub,'copyfrom HRESULT'))
                        check(dest.raw==ref.raw,(name,dim,fmt,mode,sub,'exact detiled bytes/padding'))
                        for k in range(z):
                            for y in range(count):
                                at=k*slice_+y*pitch;check(dest.raw[at:at+row_bytes]==payload[at:at+row_bytes],'roundtrip logical data')
                        check(dest.raw[-32:]==b'\xa5'*32 and tiled.raw[-32:]==b'\x7a'*32,'allocation canaries')
                    sentinel=(U*30)(*([0x12345678]*30));before=bytes(sentinel)
                    check(TILING(t[7])(p,sentinel,sentinel,sentinel,sentinel,0,sentinel)==TILING(mt[7])(mp,sentinel,sentinel,sentinel,sentinel,0,sentinel)==-2147467259,'non-tiled GetResourceTiling E_FAIL')
                    check(bytes(sentinel)==before,'non-tiled outputs untouched')
                    check(REF(t[1])(p)==0 and REF(mt[1])(mp)==0,'all genuine/proxy references released')
                    rows.append({'library':name,'dim':dim,'format':fmt,'mode':mode,'hr':0,'subresources':mips*arrays})
        # Explicit unsupported new fields refuse before original factory call.
        fn=original.XGCreateTexture2DComputer
        for index,value in [(12,1),(10,0x08000000)]:
            desc=describe(2,28,64,64,1,1,1,14);desc[index]=value;p=P(123)
            check(make(desc,C.byref(p),C.cast(fn,P))==-2147467263 and not p.value,'unproved new selector refuses')
        # Corrupt only the privately loaded test-process mapping, restore it
        # before any original call. Source game files are never written.
        win=C.WinDLL('kernel32');win.VirtualProtect.argtypes=[P,C.c_size_t,U,C.POINTER(U)];win.VirtualProtect.restype=C.c_int
        for at in [0x6fb0 if name=='xg_x.dll'else 0x6f20,0x12bc0 if name=='xg_x.dll'else 0x12ac0,0xe288+24]:
            address=original._handle+at;prior=U();restore=U();check(win.VirtualProtect(address,8,0x40,C.byref(prior))!=0,'test mapping writable')
            byte=C.c_ubyte.from_address(address);saved=byte.value
            try:
                byte.value^=1;p=P(123);desc=describe(2,28,64,64,1,1,1,14)
                check(make(desc,C.byref(p),C.cast(fn,P))<0 and not p.value,'mutated code/LUT/vtable refused before original call')
            finally:
                byte.value=saved;check(win.VirtualProtect(address,8,prior.value,C.byref(restore))!=0,'test mapping protection restored')
        for dim,fmt,w,h,z,mips,mode,samples,quality,bind in [
                (2,99,64,64,1,7,14,1,0,8),
                (3,80,128,128,128,8,13,1,0,8),
                (2,26,1920,1080,1,1,14,2,2,0x28)]:
            desc=describe(dim,fmt,w,h,z,mips,1,mode,samples,quality,bind)
            f=getattr(original,f'XGCreateTexture{dim}DComputer');p=P();hr=make(desc,C.byref(p),C.cast(f,P));mhr,mp,_=create(modern,dim,desc)
            check(hr==mhr==0,'recorded title profile factory')
            native=P.from_address(p.value+24).value
            check(C.string_at(native+12,0x6200-12)==C.string_at(mp+12,0x6200-12),'recorded title profile complete address/layout object')
            check(REF(table(p)[1])(p)==REF(table(mp)[1])(mp)==0,'recorded title profile lifetime')
            real_profiles.append({'library':name,'dimension':dim,'format':fmt,'width':w,'height':h,'depth':z,'mips':mips,'mode':mode,'samples':samples,'quality':quality})
        optimal=original.XGComputeOptimalTileMode;optimal.argtypes=[U]*7;optimal.restype=U
        for fmt,bind in [(10,0x28),(26,0x28),(28,0x28),(20,0x48),(45,0x48)]:
            for samples,quality in [(1,0),(2,0),(2,1),(2,2),(4,0),(4,1),(4,2),(8,0)]:
                mode=optimal(3,fmt,64,32,1,samples,bind)
                desc=describe(2,fmt,64,32,1,1,1,mode,samples,quality,bind)
                p=P();r=make(desc,C.byref(p),C.cast(fn,P));mhr,mp,_=create(modern,2,desc)
                entry={'library':name,'format':fmt,'samples':samples,'quality':quality,'mode':mode,'hr':r,'oracle_hr':mhr,'planes':[]}
                msaa_rows.append(entry)
                if r<0 or mhr<0:
                    if p.value:REF(table(p)[1])(p)
                    if mp:REF(table(mp)[1])(mp)
                    continue
                t,mt=table(p),table(mp);a=C.create_string_buffer(0xd40);b=C.create_string_buffer(0xd40)
                check(LAY(t[2])(p,a)==LAY(mt[2])(mp,b)==0,'MSAA layout HRESULT')
                check(a.raw==b.raw,(entry,'MSAA full layout'))
                resource=int.from_bytes(a.raw[:8],'little');planes=int.from_bytes(a.raw[20:24],'little')
                for plane in range(planes):
                    base=24+plane*0x460
                    get=lambda at:int.from_bytes(a.raw[at:at+4],'little')
                    usage=get(base);bpe=get(base+32);w=get(base+88);h=get(base+92)
                    pitch=w*bpe+16;slice_=pitch*h+32;extent=slice_+32
                    source=C.create_string_buffer(bytes((i*13+plane*47)&255 for i in range(extent)),extent)
                    tiled=C.create_string_buffer(b'\x7a'*(resource+32),resource+32);tiled_ref=C.create_string_buffer(tiled.raw,len(tiled))
                    ahr=COPY(t[5])(p,tiled,plane,0,source,pitch,slice_);bhr=COPY(mt[5])(mp,tiled_ref,plane,0,source,pitch,slice_)
                    check(ahr==bhr,(entry,plane,'MSAA plane CopyInto HRESULT'))
                    check(tiled.raw==tiled_ref.raw,(entry,plane,'MSAA plane tiled bytes'))
                    dest=C.create_string_buffer(b'\xa5'*extent,extent);ref=C.create_string_buffer(dest.raw,extent)
                    ahr=COPY(t[6])(p,dest,plane,0,tiled,pitch,slice_);bhr=COPY(mt[6])(mp,ref,plane,0,tiled_ref,pitch,slice_)
                    check(ahr==bhr,(entry,plane,usage,'MSAA plane CopyFrom HRESULT',ahr,bhr))
                    check(dest.raw==ref.raw,(entry,plane,'MSAA plane exact detile/padding'))
                    entry['planes'].append({'plane':plane,'usage':usage,'bytes_per_element':bpe,'width_elements':w,'height_elements':h,'copy_hr':ahr})
                check(REF(t[1])(p)==REF(mt[1])(mp)==0,'MSAA lifetime')
        # Address metadata and all physical layout fields must match, not just
        # the logical-width bytes involved in the NPOT correction. Read only
        # our own proxy's legacy field (x64 offsetof Computer.legacy==24).
        for width in [5,7,9,13,19,20,31,33,63,65,127,129,255,257,260,511,513,1023,1025,1919,1920]:
            for height in [13,129,width]:
                for fmt in [71,77,80,98]:
                    for mode in [13,14]:
                        mips=max(width,height).bit_length()
                        desc=describe(2,fmt,width,height,1,mips,1,mode)
                        p=P();hr=make(desc,C.byref(p),C.cast(fn,P));mhr,mp,_=create(modern,2,desc)
                        check(hr==mhr==0,'NPOT full-chain factories')
                        native=P.from_address(p.value+24).value
                        check(P.from_address(native).value==original._handle+0xe288,'fixture own legacy pointer')
                        # Skip vtable/refcount header. Every original body field
                        # up to its allocation end must be exactly equal.
                        check(C.string_at(native+12,0x6200-12)==C.string_at(mp+12,0x6200-12),(name,width,height,fmt,mode,'NPOT complete private layout'))
                        check(REF(table(p)[1])(p)==REF(table(mp)[1])(mp)==0,'NPOT lifetime')
                        npot_checks+=1
    # Source and built module contain no oracle pathname/import/dependency.
    check(b'fm5-host-xg.dll' not in dll.read_bytes() and b'xg_x.dll' not in dll.read_bytes(),'no foreign XG import/name embedded')
    result={'passed':True,'checks':checks,'cases':rows,'msaa_cases':msaa_rows,'real_profile_layouts':real_profiles,'npot_full_private_objects':npot_checks,'elapsed_seconds':time.perf_counter()-started,
            'oracle':'LOCAL ONLY modern XG; production imports only kernel32/bcrypt/CRT',
            'limitations':['Explicit ESRAMUsageBytes nonzero unsupported','Sparse/tiled-resource flag08000000 unsupported','No engine/GPU/game execution']}
    (OUT/'result.json').write_text(json.dumps(result,indent=2)+'\n');print(json.dumps({k:v for k,v in result.items()if k not in ['cases','msaa_cases']}))
if __name__=='__main__':run()
