#include "fm5_allocation.h"
#include <string.h>

typedef HRESULT (WINAPI *PlacementBufferFn)(void *,const void *,void *,void **);
typedef union NativeCall { void *address; PlacementBufferFn placement; } NativeCall;
static const BYTE placement_buffer_guard[16]={
    0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0xac
};
static BOOL native_guard(void *method) {
#ifdef FM5_ALLOCATION_TEST
    (void)placement_buffer_guard;
    return method!=NULL;
#else
    BYTE *base=(BYTE *)GetModuleHandleW(NULL);
    if(!base || ((IMAGE_DOS_HEADER *)base)->e_magic!=IMAGE_DOS_SIGNATURE)return FALSE;
    LONG offset=((IMAGE_DOS_HEADER *)base)->e_lfanew;
    if(offset<0x40 || offset>0x1000)return FALSE;
    IMAGE_NT_HEADERS64 *pe=(IMAGE_NT_HEADERS64 *)(base+offset);
    if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC || pe->OptionalHeader.SizeOfImage<0x121880)return FALSE;
    return method==base+0x121870 && !memcmp(method,placement_buffer_guard,sizeof(placement_buffer_guard));
#endif
}
HRESULT WINAPI FM5CreateNativePlacementBuffer(void *native_device,
    const void *legacy_description28,void *placement,void **native_owned_out) {
    if(native_owned_out)*native_owned_out=NULL;
    if(!native_device || !legacy_description28 || !native_owned_out)return E_INVALIDARG;
    /* Original19b78 overrides the legacy tail with zero before creation.
     * Native121870 consumes the canonical 24-byte descriptor directly and
     * installs caller memory at native+0x20/+0x168. Never use normal CreateBuffer.
     */
    DWORD canonical[6];memcpy(canonical,legacy_description28,sizeof(canonical));
    NativeCall call;call.address=(*(void ***)native_device)[62];
    if(!native_guard(call.address))return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    return call.placement(native_device,canonical,placement,native_owned_out);
}
