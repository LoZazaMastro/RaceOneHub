/* An authored host ABI adapter over the user's original FM5 address computer.
 * Native private offset helpers run in that original module under SHA256
 * guards. This source embeds fingerprints, never their executable bytes. */
#include "fm5_portable_xg.h"
#include <bcrypt.h>
#include <stdint.h>
#include <string.h>

typedef HRESULT (WINAPI *FactoryFn)(const void *,void **);
typedef ULONG (WINAPI *RefFn)(void *);
typedef HRESULT (WINAPI *LayoutFn)(void *,void *);
typedef UINT64 (WINAPI *MipFn)(void *,UINT,UINT);
typedef UINT64 (WINAPI *ElementFn)(void *,UINT,UINT,UINT64,UINT,UINT,UINT);
typedef HRESULT (WINAPI *CopyFn)(void *,void *,UINT,UINT,const void *,UINT,UINT);
typedef INT64 (WINAPI *AddressFn)(UINT64,UINT,UINT,UINT,UINT64,UINT64,void *);
typedef struct Computer {void *const *vtable;LONG refs;SRWLOCK lock;BYTE *legacy,*module;DWORD misc,helper_delta;} Computer;
static ULONG WINAPI addref(Computer *self);
static ULONG WINAPI release(Computer *self);
static HRESULT WINAPI layout(Computer *self,void *out);
static UINT64 WINAPI mip(Computer *self,UINT plane,UINT level);
static UINT64 WINAPI element(Computer *self,UINT plane,UINT level,UINT64 x,UINT y,UINT z,UINT sample);
static HRESULT WINAPI tile(Computer *self,void *out,UINT plane,UINT subresource,const void *in,UINT row,UINT slice);
static HRESULT WINAPI untile(Computer *self,void *out,UINT plane,UINT subresource,const void *in,UINT row,UINT slice);
static HRESULT WINAPI tiling(Computer *self,UINT *tiles,void *packed,void *shape,UINT *count,UINT first,void *subresources);
static void *const methods[]={(void *)addref,(void *)release,(void *)layout,(void *)mip,(void *)element,(void *)tile,(void *)untile,(void *)tiling};
static const DWORD original_methods[]={0x12c0,0x12d0,0x1310,0x2500,0x2540,0x2610};
/* Both original FM5 libraries have no .text base relocations. Hash their full
 * executable section, including all private-helper control-flow and tables. */
static const BYTE text_hashes[2][32]={
 {0x25,0x89,0x2a,0xd6,0x4e,0x53,0x60,0x41,0xcb,0x0c,0xfd,0x0b,0x55,0x6d,0xa8,0xd8,0x5f,0x2a,0xe6,0x87,0x2c,0x2c,0x16,0xeb,0xed,0xf8,0x19,0x04,0x38,0x78,0x9e,0x3f},
 {0x59,0x80,0x5c,0x2c,0x33,0x3e,0xcf,0x0a,0xff,0x82,0x7f,0xdd,0x02,0xb7,0xd8,0x16,0xe0,0x47,0xeb,0xbc,0x13,0xb7,0x07,0xf0,0x78,0x9e,0x7f,0x9f,0xe4,0xdc,0x30,0x22}};
static const BYTE lut_hash[32]={0xba,0x61,0x6c,0x3a,0xd8,0x6e,0x82,0x97,0xd7,0x19,0xad,0xe6,0xa1,0xa9,0xcc,0x96,0x0d,0x51,0xc8,0x83,0x4c,0xa3,0xea,0x32,0xfc,0x57,0x38,0x03,0x76,0xa9,0x23,0x12};
static DWORD u32(const BYTE *p) {DWORD result;memcpy(&result,p,4);return result;}
static UINT64 u64(const BYTE *p) {UINT64 result;memcpy(&result,p,8);return result;}
static HRESULT module_guard(void *factory,BYTE **module,UINT *dimension) {
    MEMORY_BASIC_INFORMATION memory;BYTE *base;IMAGE_DOS_HEADER *dos;IMAGE_NT_HEADERS64 *nt;
    UINT version;DWORD length;BCRYPT_ALG_HANDLE provider=NULL;BYTE digest[32];NTSTATUS status;HRESULT hr=S_OK;
    if(!factory||!VirtualQuery(factory,&memory,sizeof(memory))||memory.State!=MEM_COMMIT)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    base=(BYTE *)memory.AllocationBase;
    __try {
        dos=(IMAGE_DOS_HEADER *)base;
        if(dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0x40||dos->e_lfanew>0x800)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        nt=(IMAGE_NT_HEADERS64 *)(base+dos->e_lfanew);
        if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->FileHeader.Machine!=IMAGE_FILE_MACHINE_AMD64||nt->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC||nt->OptionalHeader.SizeOfImage!=0x1b000)return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        if(nt->OptionalHeader.AddressOfEntryPoint==0xc6a8&&nt->FileHeader.TimeDateStamp==0x52820d2e){version=0;length=0xc14a;}
        else if(nt->OptionalHeader.AddressOfEntryPoint==0xc738&&nt->FileHeader.TimeDateStamp==0x52820d27){version=1;length=0xc1da;}
        else return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        if((BYTE *)factory==base+0x2a70)*dimension=1;
        else if((BYTE *)factory==base+0x2b60)*dimension=2;
        else if((BYTE *)factory==base+0x2c50)*dimension=3;
        else return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        status=BCryptOpenAlgorithmProvider(&provider,BCRYPT_SHA256_ALGORITHM,NULL,0);
        if(status>=0)status=BCryptHash(provider,NULL,0,base+0x1000,length,digest,sizeof(digest));
        if(status<0||memcmp(digest,text_hashes[version],32))hr=HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        if(SUCCEEDED(hr)) {
            status=BCryptHash(provider,NULL,0,base+(version?0x12bc0:0x12ac0),256,digest,sizeof(digest));
            if(status<0||memcmp(digest,lut_hash,32))hr=HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
        }
        if(provider){BCryptCloseAlgorithmProvider(provider,0);provider=NULL;}
        if(FAILED(hr))return hr;
        for(UINT i=0;i<ARRAYSIZE(original_methods);++i)if(*(void **)(base+0xe288+i*8)!=base+original_methods[i])return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    } __except(EXCEPTION_EXECUTE_HANDLER) {if(provider)BCryptCloseAlgorithmProvider(provider,0);return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);}
    *module=base;return S_OK;
}
static ULONG WINAPI addref(Computer *self) {return (ULONG)InterlockedIncrement(&self->refs);}
static ULONG WINAPI release(Computer *self) {
    LONG refs=InterlockedDecrement(&self->refs);
    if(!refs){((RefFn)(*(void ***)self->legacy)[1])(self->legacy);HeapFree(GetProcessHeap(),0,self);}return (ULONG)refs;
}
static HRESULT WINAPI layout(Computer *self,void *out) {
    HRESULT hr;AcquireSRWLockExclusive(&self->lock);hr=((LayoutFn)(*(void ***)self->legacy)[2])(self->legacy,out);ReleaseSRWLockExclusive(&self->lock);return hr;
}
static UINT64 WINAPI mip(Computer *self,UINT plane,UINT level) {
    UINT64 result;AcquireSRWLockExclusive(&self->lock);result=((MipFn)(*(void ***)self->legacy)[3])(self->legacy,plane,level);ReleaseSRWLockExclusive(&self->lock);return result;
}
static UINT64 WINAPI element(Computer *self,UINT plane,UINT level,UINT64 x,UINT y,UINT z,UINT sample) {
    UINT64 result;AcquireSRWLockExclusive(&self->lock);result=((ElementFn)(*(void ***)self->legacy)[4])(self->legacy,plane,level,x,y,z,sample);ReleaseSRWLockExclusive(&self->lock);return result;
}
static HRESULT WINAPI tile(Computer *self,void *out,UINT plane,UINT subresource,const void *in,UINT row,UINT slice) {
    HRESULT hr;AcquireSRWLockExclusive(&self->lock);hr=((CopyFn)(*(void ***)self->legacy)[5])(self->legacy,out,plane,subresource,in,row,slice);ReleaseSRWLockExclusive(&self->lock);return hr;
}
static HRESULT WINAPI untile(Computer *self,void *out,UINT plane,UINT subresource,const void *in,UINT row,UINT slice) {
    BYTE *old=self->legacy,*registers,*logical,*address;UINT encoded,mips,arrays,mip_index,array_index,width,height,depth,bytes;
    UINT64 resource_size,linear_extent,row_bytes;HRESULT hr=S_OK;AddressFn compute;
    if(!in||!out)return E_INVALIDARG;
    AcquireSRWLockExclusive(&self->lock);
    /* Fields and divisions are the same ones used by original CopyInto5.
     * Its private address helpers are shared by modern CopyFrom6, unlike
     * old public element4 which refuses CMASK/FMASK/HTILE planes. */
    if(!u32(old+0xc)||plane>=u32(old+0xc)||plane>=3){hr=E_INVALIDARG;goto done;}
    registers=old+plane*32;encoded=u32(registers+0x1c);
    if(!(encoded&0x80000000)||!(u32(registers+0x14)&0x3f00000)){hr=E_FAIL;goto done;}
    mips=(encoded>>16)&15;
    if(((encoded>>28)==14||(encoded>>28)==15)&&mips)mips=1;else ++mips;
    arrays=((u32(registers+0x24)>>13)&0x1fff)+1;
    if(subresource>=(UINT64)arrays*mips){hr=E_INVALIDARG;goto done;}
    mip_index=subresource%mips;array_index=subresource/mips;
    logical=old+plane*0x460+mip_index*72;
    width=u32(logical+0x5528);height=u32(logical+0x552c);
    depth=u32(old+0x61f0)==4?u32(logical+0x5530):1;
    bytes=u32(old+plane*0x460+0x54f0);resource_size=u64(old+0x54b8);
    row_bytes=(UINT64)width*bytes;
    linear_extent=(UINT64)(depth?depth-1:0)*slice+(UINT64)(height?height-1:0)*row+row_bytes;
    if(!width||!height||!depth||!bytes||bytes>16||row_bytes>row||
       (depth>1&&(UINT64)row*height>slice)||linear_extent>SIZE_MAX){hr=E_INVALIDARG;goto done;}
    address=old+0xd8+plane*0x1c18+mip_index*0x1d8;
    compute=(AddressFn)(uintptr_t)(self->module+((address[0x68]&0x80)?0x9b20:0x6fb0)-self->helper_delta);
    /* Each helper result is a fixed-point byte address with 12 fraction bits.
     * Copy only logical elements: padded linear rows/slices stay untouched. */
    for(UINT z=0;z<depth;++z)for(UINT y=0;y<height;++y)for(UINT x=0;x<width;++x) {
        INT64 fixed=compute(x,y,u32(old+0x61f0)==4?z:array_index,0,0,0,address);
        UINT64 at=(UINT64)(fixed>>12);
        if(fixed<0||at>resource_size||bytes>resource_size-at){hr=E_FAIL;goto done;}
        memcpy((BYTE *)out+(SIZE_T)z*slice+(SIZE_T)y*row+(SIZE_T)x*bytes,(const BYTE *)in+(SIZE_T)at,bytes);
    }
done:ReleaseSRWLockExclusive(&self->lock);return hr;
}
static HRESULT WINAPI tiling(Computer *self,UINT *tiles,void *packed,void *shape,UINT *count,UINT first,void *subresources) {
    (void)tiles;(void)packed;(void)shape;(void)count;(void)first;(void)subresources;
    /* Authentic modern GetResourceTiling rejects ordinary textures BEFORE
     * touching any outputs. Sparse/tiled-resource creation is not yet ported. */
    return self->misc&0x08000000?E_NOTIMPL:E_FAIL;
}
HRESULT WINAPI FM5PortableCreateHostTextureComputer(const void *description,void **out,void *original_factory) {
    BYTE *module;UINT dimension,insert,words;DWORD converted[14],misc,format;const DWORD *input=(const DWORD *)description;
    void *original=NULL;Computer *computer;HRESULT hr;
    if(!out)return E_POINTER;*out=NULL;if(!description)return E_POINTER;
    hr=module_guard(original_factory,&module,&dimension);if(FAILED(hr))return hr;
    insert=dimension==1?9:dimension==2?12:10;words=dimension==1?12:dimension==2?15:13;
    misc=input[dimension==1?7:dimension==2?10:8];
    format=input[dimension==1?3:4];
    /* Modern1D15b0 rejects block formats; its common validation also rejects
     * linear8 block textures. The older factory accepts these extra profiles,
     * so preserve the narrower host contract before invoking it. */
    if(((format>=70&&format<=84)||(format>=94&&format<=99))&&(dimension==1||input[insert+1]==8))return E_INVALIDARG;
    /* ESRAMUsageBytes was inserted before TileMode by the newer SDK. Old
     * FM5 has no equivalent explicit range selector; do not silently drop it.
     * Tiled/sparse resource semantics likewise require their full contract. */
    if(input[insert]||(misc&0x08000000))return E_NOTIMPL;
    memcpy(converted,input,insert*4);memcpy(converted+insert,input+insert+1,(words-insert-1)*4);
    /* Original1D1460 passes ArraySize+1 to its register encoder; modern15b0
     * encodes the declared count. Compensate the former's extra element. */
    if(dimension==1){if(!converted[2])return E_INVALIDARG;--converted[2];}
    hr=((FactoryFn)original_factory)(converted,&original);
    if(FAILED(hr))return hr;
    if(!original)return E_UNEXPECTED;
    if(*(void **)original!=module+0xe288){((RefFn)(*(void ***)original)[1])(original);return E_UNEXPECTED;}
    if((format>=70&&format<=84)||(format>=94&&format<=99)) {
        BYTE *owned=(BYTE *)original;DWORD levels=u32(owned+0x54c8);
        if(u32(owned+0xc)!=1||levels>15){((RefFn)(*(void ***)original)[1])(original);return E_NOTIMPL;}
        /* SDK9894 compact logical dimensions shift an already rounded block
         * count. The host ABI rounds each mip's pixel extent instead. Full
         * private-object differential checks establish that physical packing,
         * helper metadata, offsets and allocation sizes are unchanged. Fix
         * ONLY this new owned computer's two reported/copied logical extents. */
        for(DWORD level=0;level<levels;++level) {
            DWORD width=input[0]>>level,height=dimension==1?1:input[1]>>level;
            if(!width)width=1;if(!height)height=1;
            width=(width+3)/4;height=(height+3)/4;
            memcpy(owned+0x5528+level*72,&width,4);memcpy(owned+0x552c+level*72,&height,4);
        }
    }
    computer=(Computer *)HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,sizeof(*computer));
    if(!computer){((RefFn)(*(void ***)original)[1])(original);return E_OUTOFMEMORY;}
    computer->vtable=methods;computer->refs=1;computer->legacy=(BYTE *)original;computer->module=module;computer->misc=misc;
    computer->helper_delta=((IMAGE_NT_HEADERS64 *)(module+((IMAGE_DOS_HEADER *)module)->e_lfanew))->OptionalHeader.AddressOfEntryPoint==0xc6a8?0x90:0;
    InitializeSRWLock(&computer->lock);*out=computer;return hr;
}
