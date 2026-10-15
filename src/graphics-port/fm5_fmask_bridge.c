#include "fm5_fmask_bridge.h"
#include <stdint.h>
#include <limits.h>
#include <string.h>
#include "../allocation-port/fm5_msaa4.h"

HRESULT WINAPI FM5GetLegacyTexture2DDescription(void *resource, void *out48);
HRESULT WINAPI FM5CreateLegacyResource(void *device, UINT slot, const void *description, const void *initial, void **out);
typedef HRESULT (WINAPI *CreateView)(void *, void *, const void *, void **);
typedef ULONG (WINAPI *Release)(void *);
typedef struct InitialData { const void *data; UINT row_pitch, slice_pitch; } InitialData;
#ifdef FM5_FMASK_TEST
static volatile LONG allocation_failure=-1;
void FM5FMaskFailAllocation(LONG countdown) { InterlockedExchange(&allocation_failure,countdown); }
#endif

HRESULT FM5SelectExpandedFMask(const UINT texture[12], const UINT srv[6], FM5FMaskPlan *plan) {
    UINT samples, quality;
    if (!texture || !plan) return E_POINTER;
    if (FM5ForcedSampleCount()) return FM5SelectFourSampleFMask(texture,srv,plan);
    memset(plan,0,sizeof(*plan));
    if (!srv) return S_FALSE;
    if (srv[1]!=4 || (srv[0]!=17 && srv[0]!=42 && srv[0]!=57 && srv[0]!=62) ||
        texture[5]<=1 || !(texture[8]&0x20) || (texture[8]&0x40)) return S_FALSE;
    /* This is the original driver's explicit FMask image-view family.
     * No ordinary color SRV reinterpretation is silently substituted. */
    if (srv[2]!=0 || srv[3]!=1 || srv[4]!=0 || srv[5]!=0 || texture[2]!=1 || texture[3]!=1)
        return E_NOTIMPL;
    samples=texture[5]; quality=texture[6];
    if (samples==2 && quality==1 && srv[0]==62) {
        plan->expanded_value=0x02; plan->element_bytes=1;
    } else if (samples==4 && quality>=1 && quality<=2 && srv[0]==62) {
        plan->expanded_value=0xe4; plan->element_bytes=1;
    } else if (samples==8 && quality>=1 && quality<=3 && srv[0]==42) {
        plan->expanded_value=0x76543210; plan->element_bytes=4;
    } else return E_NOTIMPL;
    if (!texture[0] || !texture[1] || texture[0]>16384 || texture[1]>16384) return E_INVALIDARG;
    plan->width=texture[0]; plan->height=texture[1]; plan->format=srv[0];
    plan->fragments=samples; plan->coverage_samples=samples;
    return S_OK;
}

static BOOL guard(void *method, UINT rva, const BYTE signature[16]) {
#ifdef FM5_FMASK_TEST
    (void)rva; (void)signature; return method!=NULL;
#else
    const BYTE *base=(const BYTE *)GetModuleHandleW(NULL);
    const IMAGE_DOS_HEADER *dos=(const IMAGE_DOS_HEADER *)base;
    const IMAGE_NT_HEADERS64 *pe;
    if (!base || dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<0x40 || dos->e_lfanew>0x1000) return FALSE;
    pe=(const IMAGE_NT_HEADERS64 *)(base+dos->e_lfanew);
    return pe->Signature==IMAGE_NT_SIGNATURE && pe->FileHeader.Machine==IMAGE_FILE_MACHINE_AMD64 &&
        pe->OptionalHeader.Magic==IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
        pe->OptionalHeader.SizeOfImage>=rva+16 && method==base+rva && !memcmp(method,signature,16);
#endif
}

HRESULT WINAPI FM5CreateLegacyShaderResourceView(void *device, void *resource,
    const void *description, void **out) {
    static const BYTE texture_signature[16]={0x48,0x8b,0xc4,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48};
    static const BYTE view_signature[16]={0x40,0x55,0x53,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57,0x48,0x8d,0x6c};
    UINT texture[12]={0}, auxiliary_desc[12]={0};
    FM5FMaskPlan plan;
    HRESULT hr;
    void **table, *auxiliary=NULL, *owned_view=NULL;
    CreateView create_view;
    SIZE_T row_bytes, bytes;
    BYTE *pixels;
    InitialData initial;
    if (out) *out=NULL;
    if (!device) return E_POINTER;
    table=*(void ***)device;
    if (!guard(table[7],0x13fe60,view_signature)) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    create_view=(CreateView)table[7];
    if (!resource || !description) return create_view(device,resource,description,out);
    hr=FM5GetLegacyTexture2DDescription(resource,texture);
    if (hr==HRESULT_FROM_WIN32(ERROR_NOT_FOUND) || hr==E_NOINTERFACE)
        return create_view(device,resource,description,out);
    if (FAILED(hr)) return hr;
    hr=FM5SelectExpandedFMask(texture,(const UINT *)description,&plan);
    if (hr==S_FALSE) return create_view(device,resource,description,out);
    if (FAILED(hr)) return hr;
    if (!guard(table[5],0xea000,texture_signature)) return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    row_bytes=(SIZE_T)plan.width*plan.element_bytes;
    bytes=row_bytes*plan.height;
    if (row_bytes>UINT_MAX || bytes>UINT_MAX) return E_INVALIDARG;
#ifdef FM5_FMASK_TEST
    {
        LONG n=InterlockedCompareExchange(&allocation_failure,-1,-1);
        if(n>=0) { if(!n) {InterlockedExchange(&allocation_failure,-1); return E_OUTOFMEMORY;} InterlockedDecrement(&allocation_failure); }
    }
#endif
    pixels=(BYTE *)HeapAlloc(GetProcessHeap(),0,bytes);
    if (!pixels) return E_OUTOFMEMORY;
    if (plan.element_bytes==1) memset(pixels,(int)plan.expanded_value,bytes);
    else for (SIZE_T i=0;i<bytes/4;++i) ((UINT *)pixels)[i]=plan.expanded_value;
    auxiliary_desc[0]=plan.width; auxiliary_desc[1]=plan.height;
    auxiliary_desc[2]=1; auxiliary_desc[3]=1; auxiliary_desc[4]=plan.format;
    auxiliary_desc[5]=1; auxiliary_desc[8]=8; /* default GPU texture, SRV bind only */
    initial.data=pixels; initial.row_pitch=(UINT)row_bytes; initial.slice_pitch=(UINT)bytes;
    /* Register the auxiliary resource too: native GetResource exposes it and
     * the shared legacy GetDesc slot11 must have matching metadata. */
    hr=FM5CreateLegacyResource(device,5,auxiliary_desc,&initial,&auxiliary);
    HeapFree(GetProcessHeap(),0,pixels);
    if (FAILED(hr) || !auxiliary) {
        if (auxiliary) ((Release)(*(void ***)auxiliary)[2])(auxiliary);
        return FAILED(hr)?hr:E_UNEXPECTED;
    }
    hr=create_view(device,auxiliary,description,out?&owned_view:NULL);
    ((Release)(*(void ***)auxiliary)[2])(auxiliary);
    if (FAILED(hr)) {
        if (owned_view) ((Release)(*(void ***)owned_view)[2])(owned_view);
        return hr;
    }
    if (out && !owned_view) return E_UNEXPECTED;
    if (out) *out=owned_view;
    return hr;
}
