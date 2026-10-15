#include "fm5_scissor_state.h"
#include <assert.h>
#include <stdio.h>
#include <limits.h>

static BYTE *context,*device;
static UINT waits,next_count;
static RECT next_rects[16];
static void *sampler_bindings[16];
static ULONG sampler_addrefs;
static ULONG sampler_addref(void *value) {
    BYTE *object=(BYTE*)value;
    if(context[0xc48])assert(*(UINT*)(context+0xd6a0)==GetCurrentThreadId());
    ++sampler_addrefs;return ++*(ULONG*)(object+8);
}
static void wait_samplers(void *owner,UINT64 sequence) {
    assert(owner==device&&sequence==17);++waits;
    CopyMemory(context+0x6820,sampler_bindings,sizeof(sampler_bindings));
}
static UINT locks;
static FM5ScissorLockRecord *lock_recorder(FM5ScissorLockRecord *record,void *value) {
    BYTE *object=(BYTE*)value,*mutex=object+0xd698;
    assert(value==context);
    if(*(UINT*)(mutex+8)==GetCurrentThreadId())++*(UINT*)(mutex+12);
    else {AcquireSRWLockExclusive((PSRWLOCK)mutex);*(UINT*)(mutex+8)=GetCurrentThreadId();*(UINT*)(mutex+12)=1;}
    record->mutex=(PSRWLOCK)mutex;record->held=1;++locks;return record;
}
static HANDLE writer_started;
static DWORD WINAPI writer(void *value) {
    BYTE *object=(BYTE*)value,*mutex=object+0xd698;
    AcquireSRWLockExclusive((PSRWLOCK)mutex);
    *(UINT*)(mutex+8)=GetCurrentThreadId();*(UINT*)(mutex+12)=1;
    SetEvent(writer_started);
    for(UINT i=0;i<16;++i) {((RECT*)(object+0x7140))[i]=next_rects[i];Sleep(1);}
    *(UINT*)(object+0x7244)=16;
    *(UINT*)(mutex+8)=0;*(UINT*)(mutex+12)=0;ReleaseSRWLockExclusive((PSRWLOCK)mutex);
    return 0;
}
static void wait_recorder(void *owner,UINT64 sequence) {
    assert(owner==device && sequence==17 && *(UINT64*)(device+0x1040)==999);
    ++waits;
    CopyMemory(context+0x7140,next_rects,sizeof(next_rects));
    *(UINT*)(context+0x7244)=next_count;
    *(UINT64*)(device+0x1180)=sequence;
}
static BYTE *load_image(void) {
    HANDLE file=CreateFileW(L"..\\..\\runtime\\azimuth-0.9.9-original\\azimuth.exe",GENERIC_READ,
        FILE_SHARE_READ,NULL,OPEN_EXISTING,0,NULL);assert(file!=INVALID_HANDLE_VALUE);
    DWORD size=GetFileSize(file,NULL),read;
    BYTE *raw=HeapAlloc(GetProcessHeap(),0,size);assert(raw);
    assert(ReadFile(file,raw,size,&read,NULL)&&read==size);CloseHandle(file);
    IMAGE_DOS_HEADER *dos=(IMAGE_DOS_HEADER*)raw;assert(dos->e_magic==IMAGE_DOS_SIGNATURE);
    IMAGE_NT_HEADERS64 *pe=(IMAGE_NT_HEADERS64*)(raw+dos->e_lfanew);assert(pe->Signature==IMAGE_NT_SIGNATURE);
    BYTE *image=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,pe->OptionalHeader.SizeOfImage);assert(image);
    IMAGE_SECTION_HEADER *section=IMAGE_FIRST_SECTION(pe);
    for(UINT i=0;i<pe->FileHeader.NumberOfSections;++i)
        CopyMemory(image+section[i].VirtualAddress,raw+section[i].PointerToRawData,section[i].SizeOfRawData);
    HeapFree(GetProcessHeap(),0,raw);return image;
}
int main(void) {
    BYTE *image=load_image();
    device=HeapAlloc(GetProcessHeap(),HEAP_ZERO_MEMORY,0x20000);assert(device);
    context=device+0x6f40;
    void *table[270]={0};table[3]=image+0xdbe90;table[45]=image+0x1b4ef0;
    *(void***)context=table;*(BYTE**)(context+0xc40)=device;
    *(UINT64*)(device+0x1040)=999;*(UINT64*)(device+0x1080)=17;
    for(UINT i=0;i<16;++i) {next_rects[i].left=-(LONG)i;next_rects[i].top=33;next_rects[i].right=1920;next_rects[i].bottom=1080;}
    RECT guarded[18];UINT count;
    for(next_count=0;next_count<=16;++next_count) {
        FillMemory(guarded,sizeof(guarded),0xcd);count=0xabcdef;
        *(UINT*)(context+0x7244)=0xffffffff;
        assert(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)==S_OK);
        assert(count==next_count && !memcmp(guarded+1,next_rects,count*sizeof(RECT)));
        for(UINT i=0;i<sizeof(RECT);++i)assert(((BYTE*)guarded)[i]==0xcd&&((BYTE*)(guarded+17))[i]==0xcd);
    }
    UINT before=waits; const UINT rvas[]={0x1b4ef0,0x1b4e40,0xef960,0x1b61f0,0xdbe90,0x589b0,0xc2f00};
    for(UINT i=0;i<7;++i)for(UINT j=0;j<32;++j) {
        image[rvas[i]+j]^=0x80;count=0xabcdef;FillMemory(guarded,sizeof(guarded),0xcd);
        assert(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT));
        assert(count==0xabcdef&&waits==before&&((BYTE*)guarded)[16]==0xcd);image[rvas[i]+j]^=0x80;
    }
    table[45]=NULL;assert(FAILED(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)));table[45]=image+0x1b4ef0;
    table[3]=NULL;assert(FAILED(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)));table[3]=image+0xdbe90;
    context[0xc48]=1;assert(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)==E_UNEXPECTED);context[0xc48]=0;
    context[0xc48]=1;context[0xc49]=1;assert(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)==E_NOTIMPL);context[0xc48]=0;context[0xc49]=0;
    assert(waits==before);
    next_count=1;context[0xc49]=1;assert(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)==S_OK);context[0xc49]=0;
    *(BYTE**)(context+0xc40)=device+8;assert(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT));
    *(BYTE**)(context+0xc40)=NULL;assert(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)==E_UNEXPECTED);*(BYTE**)(context+0xc40)=device;
    next_count=17;count=0xabcdef;assert(FM5TestReadNativeScissorSnapshot(context,&count,guarded+1,image,wait_recorder)==HRESULT_FROM_WIN32(ERROR_INVALID_DATA));assert(count==0xabcdef);
    assert(FM5TestReadNativeScissorSnapshot(NULL,&count,guarded+1,image,wait_recorder)==E_POINTER);
    assert(FM5TestReadNativeScissorSnapshot(context,NULL,guarded+1,image,wait_recorder)==E_POINTER);
    assert(FM5TestReadNativeScissorSnapshot(context,&count,NULL,image,wait_recorder)==E_POINTER);
    /* Non-recording deferred contexts use their native recursive SRW guard,
     * with no immediate-worker drain and no changes to state. */
    context[0xc48]=1;context[0xc49]=0;
    *(UINT*)(context+0x7244)=2;CopyMemory(context+0x7140,next_rects,sizeof(next_rects));
    before=waits;
    assert(FM5TestReadNativeScissorSnapshotLocked(context,&count,guarded+1,image,wait_recorder,lock_recorder)==S_OK);
    assert(count==2&&waits==before&&locks==1&&!memcmp(guarded+1,next_rects,2*sizeof(RECT)));
    BYTE *mutex=context+0xd698;
    assert(!*(UINT*)(mutex+8)&&!*(UINT*)(mutex+12));
    assert(TryAcquireSRWLockExclusive((PSRWLOCK)mutex));ReleaseSRWLockExclusive((PSRWLOCK)mutex);
    AcquireSRWLockExclusive((PSRWLOCK)mutex);*(UINT*)(mutex+8)=GetCurrentThreadId();*(UINT*)(mutex+12)=1;
    assert(FM5TestReadNativeScissorSnapshotLocked(context,&count,guarded+1,image,wait_recorder,lock_recorder)==S_OK);
    assert(*(UINT*)(mutex+8)==GetCurrentThreadId()&&*(UINT*)(mutex+12)==1&&locks==2);
    *(UINT*)(mutex+8)=0;*(UINT*)(mutex+12)=0;ReleaseSRWLockExclusive((PSRWLOCK)mutex);
    *(UINT*)(context+0x7244)=17;count=0xabcdef;
    assert(FM5TestReadNativeScissorSnapshotLocked(context,&count,guarded+1,image,wait_recorder,lock_recorder)==HRESULT_FROM_WIN32(ERROR_INVALID_DATA));assert(count==0xabcdef);
    assert(TryAcquireSRWLockExclusive((PSRWLOCK)mutex));ReleaseSRWLockExclusive((PSRWLOCK)mutex);
    *(UINT*)(context+0x7244)=1;
    BYTE *inaccessible=VirtualAlloc(NULL,4096,MEM_RESERVE|MEM_COMMIT,PAGE_NOACCESS);assert(inaccessible);
    BOOL caught=FALSE;
    __try {FM5TestReadNativeScissorSnapshotLocked(context,&count,inaccessible,image,wait_recorder,lock_recorder);}
    __except(GetExceptionCode()==EXCEPTION_ACCESS_VIOLATION?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH) {caught=TRUE;}
    assert(caught&&TryAcquireSRWLockExclusive((PSRWLOCK)mutex));ReleaseSRWLockExclusive((PSRWLOCK)mutex);VirtualFree(inaccessible,0,MEM_RELEASE);
    writer_started=CreateEventW(NULL,TRUE,FALSE,NULL);assert(writer_started);
    HANDLE thread=CreateThread(NULL,0,writer,context,0,NULL);assert(thread);
    assert(WaitForSingleObject(writer_started,5000)==WAIT_OBJECT_0);
    assert(FM5TestReadNativeScissorSnapshotLocked(context,&count,guarded+1,image,wait_recorder,lock_recorder)==S_OK);
    assert(count==16&&!memcmp(guarded+1,next_rects,sizeof(next_rects))&&waits==before);
    assert(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0);CloseHandle(thread);CloseHandle(writer_started);
    BYTE objects[2][0x60]={0};
    for(UINT i=0;i<2;++i) {*(ULONG*)(objects[i]+8)=1;*(BYTE**)(objects[i]+0x30)=device;*(UINT*)(objects[i]+0x38)=0xa;}
    for(UINT i=0;i<16;++i)sampler_bindings[i]=i%3==0?NULL:objects[i%2];
    table[11]=image+0xf5cb0;
    UINT sampler_cases=0;
    for(UINT deferred=0;deferred<2;++deferred) {
        context[0xc48]=(BYTE)deferred;
        for(UINT start=0;start<=16;++start)for(UINT n=0;n<=16-start;++n) {
            void *out[18];FillMemory(out,sizeof(out),0xcd);
            if(deferred)CopyMemory(context+0x6820,sampler_bindings,sizeof(sampler_bindings));
            else ZeroMemory(context+0x6820,sizeof(sampler_bindings));
            assert(FM5TestGetNativePixelSamplers(context,start,n,out+1,image,wait_samplers,lock_recorder,sampler_addref)==S_OK);
            for(UINT i=0;i<n;++i) {
                assert(out[i+1]==sampler_bindings[start+i]);
                if(out[i+1])--*(ULONG*)((BYTE*)out[i+1]+8);
            }
            assert(*(ULONG*)(objects[0]+8)==1&&*(ULONG*)(objects[1]+8)==1);
            assert(out[0]==(void*)(UINT_PTR)0xcdcdcdcdcdcdcdcdull&&out[n+1]==(void*)(UINT_PTR)0xcdcdcdcdcdcdcdcdull);
            ++sampler_cases;
        }
    }
    before=waits;ULONG refs_before=sampler_addrefs;void *out[2]={(void*)1,(void*)2};
    assert(FM5TestGetNativePixelSamplers(context,UINT_MAX,UINT_MAX,NULL,image,wait_samplers,lock_recorder,sampler_addref)==S_OK);
    assert(FM5TestGetNativePixelSamplers(context,16,0,(void**)1,image,wait_samplers,lock_recorder,sampler_addref)==S_OK);
    assert(FM5TestGetNativePixelSamplers(context,16,1,out,image,wait_samplers,lock_recorder,sampler_addref)==E_INVALIDARG);
    assert(FM5TestGetNativePixelSamplers(context,UINT_MAX,1,out,image,wait_samplers,lock_recorder,sampler_addref)==E_INVALIDARG);
    assert(waits==before&&sampler_addrefs==refs_before&&out[0]==(void*)1&&out[1]==(void*)2);
    const UINT sampler_rvas[]={0xf5cb0,0x1b29a0,0xda960};
    for(UINT i=0;i<3;++i)for(UINT j=0;j<32;++j) {
        image[sampler_rvas[i]+j]^=0x80;
        assert(FM5TestGetNativePixelSamplers(context,1,1,out,image,wait_samplers,lock_recorder,sampler_addref)==HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT));
        image[sampler_rvas[i]+j]^=0x80;
    }
    assert(waits==before&&sampler_addrefs==refs_before);
    context[0xc49]=1;
    assert(FM5TestGetNativePixelSamplers(context,1,1,out,image,wait_samplers,lock_recorder,sampler_addref)==E_NOTIMPL);context[0xc49]=0;
    *(UINT*)(objects[1]+0x38)=0xb;
    assert(FM5TestGetNativePixelSamplers(context,1,1,out,image,wait_samplers,lock_recorder,sampler_addref)==HRESULT_FROM_WIN32(ERROR_INVALID_DATA));*(UINT*)(objects[1]+0x38)=0xa;
    *(BYTE**)(objects[1]+0x30)=NULL;
    assert(FM5TestGetNativePixelSamplers(context,1,1,out,image,wait_samplers,lock_recorder,sampler_addref)==HRESULT_FROM_WIN32(ERROR_INVALID_DATA));*(BYTE**)(objects[1]+0x30)=device;
    assert(sampler_addrefs==refs_before&&out[0]==(void*)1&&out[1]==(void*)2);
    assert(TryAcquireSRWLockExclusive((PSRWLOCK)mutex));ReleaseSRWLockExclusive((PSRWLOCK)mutex);
    HeapFree(GetProcessHeap(),0,device);HeapFree(GetProcessHeap(),0,image);
    puts("PASS: actual PE fingerprints,224 guard mutations rejected before wait, published-not-produced sequence,0..16 immediate rectangles, deferred native-lock ABI/reentrancy/contention/error+exception cleanup, recording/wrong-interface/owner/NULL/corrupt-count rejection");
    printf("PASS: %u immediate/deferred sampler ranges, actual cached identities/NULLs and owned AddRefs,96 sampler guard mutations, zero-count/NULL output, bounds/wrong kind/owner/recording rejection, lock cleanup\n",sampler_cases);
    return 0;
}
