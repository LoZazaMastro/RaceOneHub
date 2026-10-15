#include "fm5_msaa4.h"
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "fm5_resolve_original_guard.inc"
#include "fm5_resolve_four_samples.inc"
#include "fm5_resolve_eight_samples.inc"

typedef HRESULT (WINAPI *CreatePixelShader)(void *, const void *, SIZE_T, void *, void **);
static FM5PixelShaderObserver pixel_shader_observer;
void FM5ConfigurePixelShaderObserver(FM5PixelShaderObserver observer) { pixel_shader_observer=observer; }
typedef struct Chunk { const BYTE *data; UINT size; } Chunk;
typedef struct ShaderGuard {
    const BYTE *rdef,*isgn,*osgn,*code,*replacement;
    UINT rdef_bytes,isgn_bytes,osgn_bytes,code_bytes,replacement_bytes;
} ShaderGuard;
#define SHADER_GUARD(id,translated) {original_##id##_rdef,original_##id##_isgn,original_##id##_osgn,original_##id##_shex,translated,sizeof(original_##id##_rdef),sizeof(original_##id##_isgn),sizeof(original_##id##_osgn),sizeof(original_##id##_shex),sizeof(translated)}
static const ShaderGuard shader_guards[] = {
    SHADER_GUARD(11d41d0,fm5_resolve_four_samples),
    SHADER_GUARD(14a6140,fm5_resolve_four_samples),
    SHADER_GUARD(171a050,fm5_resolve_four_samples_tonemap),
    SHADER_GUARD(18a68b0,fm5_resolve_four_samples_tonemap)
};
#undef SHADER_GUARD
#ifdef FM5_MSAA4_TEST
static const BYTE *test_host;
void FM5SetMsaaTestHost(const BYTE *host) { test_host = host; }
#endif

UINT FM5ForcedSampleCount(void) {
#ifdef FM5_MSAA4_TEST
    const BYTE *host = test_host;
#else
    const BYTE *host = (const BYTE *)GetModuleHandleW(NULL);
#endif
    static const UINT sites[] = {0xe752a,0x130a4e,0x132310,0x1a7e60,0x1ab026};
    if (!host || ((const IMAGE_DOS_HEADER *)host)->e_magic != IMAGE_DOS_SIGNATURE) return FALSE;
    LONG offset = ((const IMAGE_DOS_HEADER *)host)->e_lfanew;
    if (offset <= 0 || offset > 4096) return FALSE;
    const IMAGE_NT_HEADERS64 *pe = (const IMAGE_NT_HEADERS64 *)(host + offset);
    if (pe->Signature != IMAGE_NT_SIGNATURE || pe->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        pe->OptionalHeader.SizeOfImage < 0x318ea4) return 0;
    UINT count=*(const UINT *)(host+0x318ea0);
    if(count!=4 && count!=8)return 0;
    for (UINT i = 0; i < _countof(sites); ++i)
        if (host[sites[i]] != 0x90 || host[sites[i] + 1] != 0x90) return FALSE;
    for (UINT i = 0; i < 6; ++i) if (host[0xe7551 + i] != 0x90) return FALSE;
    return count;
}

BOOL FM5ForcedFourSampleMode(void) { return FM5ForcedSampleCount()==4; }

/* Original parser stores numeric sample count; its native writer formats
   any count as msaaxN. This guarded callsite adapter adds exact msaax8. */
__declspec(dllexport) UINT WINAPI FM5ParseNativeAntialiasing(const void *wstring,UINT fallback) {
    if(!wstring)return fallback;
    const BYTE *s=(const BYTE*)wstring;SIZE_T length,capacity;
    memcpy(&length,s+16,sizeof(length));memcpy(&capacity,s+24,sizeof(capacity));
    if(length>capacity || capacity>1048576 || (length!=3 && length!=6))return fallback;
    const wchar_t *text;
    if(capacity>7)memcpy(&text,s,sizeof(text));else text=(const wchar_t*)s;
    if(!text)return fallback;
    if(length==3 && !memcmp(text,L"off",6))return 1;
    if(length!=6 || memcmp(text,L"msaax",10))return fallback;
    return text[5]==L'2'?2:text[5]==L'4'?4:text[5]==L'8'?8:fallback;
}

static UINT original_native_mask(UINT effective,UINT original,UINT mask) {
    if(effective==original)return mask;
    if(!effective || effective>16 || !original || original>16) {
        RaiseException(0xe0f5000d,EXCEPTION_NONCONTINUABLE,0,NULL);
        ExitProcess(ERROR_INVALID_DATA);
    }
    UINT result=mask & (0xffffffffu<<effective);
    for(UINT i=0;i<original;++i)if(mask&(1u<<i)) {
        UINT target=effective>=4?i:effective==2?(~i&1):0;
        result|=1u<<target;
    }
    return result;
}

__declspec(dllexport) UINT WINAPI FM5MapNativeSampleMask(UINT effective_count,
    UINT original_count,UINT original_quality,UINT original_mask) {
    if((effective_count!=4 && effective_count!=8) || FM5ForcedSampleCount()!=effective_count)
        return original_native_mask(effective_count,original_count,original_mask);
    UINT coverage=0;
    if(original_count==2 && original_quality<=4)
        coverage=original_quality<=1?2:1u<<original_quality;
    else if(original_count==4 && original_quality<=4)
        coverage=original_quality<=2?4:1u<<original_quality;
    else if(original_count==8 && original_quality<=4)
        coverage=original_quality<=3?8:16;
    else if(original_count==16 && original_quality==0)coverage=16;
    if(effective_count==8) {
        if(coverage==8)return original_mask;
        if(coverage==2 || coverage==4) {
            /* Nearest standard4 sample region at each standard8 position:
               0,3,1,0,2,2,3,1. For2 coverage compose the pinned native
               standard2-to4 mapping; balanced boundary ties stay stable. */
            static const UINT regions[4]={0x09,0x84,0x30,0x42};
            UINT mask=original_mask,result=original_mask&~0xffu;
            if(coverage==2)mask=((mask&1)?0xau:0u)|((mask&2)?5u:0u);
            for(UINT i=0;i<4;++i)if(mask&(1u<<i))result|=regions[i];
            return result;
        }
        if(coverage==16) {
            UINT bits=original_mask&0xffffu;
            if(!bits)return original_mask&~0xffu;
            if(bits==0xffffu)return original_mask|0xffu;
        }
        ULONG_PTR details[]={effective_count,original_count,original_quality,original_mask};
        RaiseException(0xe0f5000d,EXCEPTION_NONCONTINUABLE,4,details);
        ExitProcess(ERROR_NOT_SUPPORTED);
    }
    if(coverage==2) {
        /* Inverse of the pinned native4-to2 standard-position mapping:
           native130ab7 maps old4 indices0/2->1 and1/3->0. */
        UINT low=((original_mask&1)?0xau:0u)|((original_mask&2)?5u:0u);
        return (original_mask&~0xfu)|low;
    }
    /* Original UMD6f1f0 writes low16 sample bits directly to both AA_MASK
       registers. Count2/quality2 is FOUR coverage samples, not two bits. */
    if(coverage==4)return original_mask;
    if(coverage>4) {
        UINT covered=original_mask&((1u<<coverage)-1u);
        if(!covered)return original_mask&~0xfu;
        if(covered==((1u<<coverage)-1u))return original_mask|0xfu;
    }
    /* No invented correspondence for partial8/16-coverage Xbox patterns. */
    ULONG_PTR details[]={effective_count,original_count,original_quality,original_mask};
    RaiseException(0xe0f5000d,EXCEPTION_NONCONTINUABLE,4,details);
    ExitProcess(ERROR_NOT_SUPPORTED);
}

static BOOL shader_chunks(const void *source, SIZE_T bytes, Chunk *rdef, Chunk *isgn, Chunk *osgn, Chunk *code) {
    if (!source || bytes < 32 || bytes > 1048576) return FALSE;
    const BYTE *data = (const BYTE *)source;
    UINT total, count;
    memcpy(&total, data + 24, 4); memcpy(&count, data + 28, 4);
    if (memcmp(data,"DXBC",4) || total != bytes || count > 32 || 32 + count * 4 > total) return FALSE;
    for (UINT i = 0; i < count; ++i) {
        UINT offset, size;
        memcpy(&offset,data + 32 + i * 4,4);
        if (offset > total - 8) return FALSE;
        memcpy(&size,data + offset + 4,4);
        if (size > total - offset - 8) return FALSE;
        Chunk *chunk = NULL;
        if (!memcmp(data + offset,"RDEF",4)) chunk = rdef;
        else if (!memcmp(data + offset,"ISGN",4)) chunk = isgn;
        else if (!memcmp(data + offset,"OSGN",4)) chunk = osgn;
        else if (!memcmp(data + offset,"SHEX",4)) chunk = code;
        if (chunk) {
            if (chunk->data) return FALSE;
            chunk->data = data + offset + 8; chunk->size = size;
        }
    }
    return rdef->data && isgn->data && osgn->data && code->data;
}

HRESULT FM5SelectFourSampleResolve(const void *original, SIZE_T bytes, UINT requested_samples,
                                  const void **replacement, SIZE_T *replacement_bytes) {
    if (!replacement || !replacement_bytes) return E_POINTER;
    *replacement = original; *replacement_bytes = bytes;
    if (requested_samples != 4 && requested_samples != 8) return S_FALSE;
    Chunk rdef = {0}, isgn = {0}, osgn = {0}, code = {0};
    if (!shader_chunks(original,bytes,&rdef,&isgn,&osgn,&code)) return S_FALSE;
    /* Full byte equality of all executable/reflection/signature chunks is
       stricter than a hash match. Only debug source/annotations may differ. */
    for (UINT i=0;i<_countof(shader_guards);++i) {
        const ShaderGuard *guard=&shader_guards[i];
        if (rdef.size != guard->rdef_bytes || memcmp(rdef.data,guard->rdef,rdef.size) ||
            isgn.size != guard->isgn_bytes || memcmp(isgn.data,guard->isgn,isgn.size) ||
            osgn.size != guard->osgn_bytes || memcmp(osgn.data,guard->osgn,osgn.size) ||
            code.size != guard->code_bytes || memcmp(code.data,guard->code,code.size)) continue;
        if(requested_samples==8) {
            *replacement=i<2?(const void*)fm5_resolve_eight_samples:(const void*)fm5_resolve_eight_samples_tonemap;
            *replacement_bytes=i<2?sizeof(fm5_resolve_eight_samples):sizeof(fm5_resolve_eight_samples_tonemap);
        } else { *replacement=guard->replacement;*replacement_bytes=guard->replacement_bytes; }
        return S_OK;
    }
    BOOL color=FALSE,mask=FALSE;
    for(UINT i=0;i<rdef.size;++i) {
        if(rdef.size-i>=13 && !memcmp(rdef.data+i,"TexColorMSAA",13))color=TRUE;
        if(rdef.size-i>=9 && !memcmp(rdef.data+i,"TexFMask",9))mask=TRUE;
    }
    /* An unknown member of this semantic family cannot retain a resolver
       which might read fewer fragments than the forced physical surface. */
    return color && mask ? E_NOTIMPL : S_FALSE;
}

HRESULT FM5SelectFourSampleFMask(const UINT texture[12], const UINT srv[6], FM5FMaskPlan *plan) {
    if (!texture || !plan) return E_POINTER;
    memset(plan,0,sizeof(*plan));
    if (!srv || srv[1] != 4 || (srv[0]!=17 && srv[0]!=42 && srv[0]!=57 && srv[0]!=62) || texture[5] <= 1 ||
        !(texture[8] & 0x20) || (texture[8] & 0x40)) return S_FALSE;
    if (srv[2] != 0 || srv[3] != 1 || srv[4] || srv[5] || texture[2] != 1 || texture[3] != 1)
        return E_NOTIMPL;
    if ((texture[5]==2 || texture[5]==4) && texture[6]>=1 && texture[6]<=2 && srv[0]==62) {
        plan->element_bytes=1;plan->expanded_value=0xe4;
    } else if (((texture[5]==4 && texture[6]==3) || (texture[5]==8 && texture[6]>=1 && texture[6]<=3)) && srv[0]==42) {
        plan->element_bytes=4;plan->expanded_value=0x44443210;
    } else return E_NOTIMPL;
    if (!texture[0] || !texture[1] || texture[0] > 16384 || texture[1] > 16384) return E_INVALIDARG;
    plan->width = texture[0]; plan->height = texture[1]; plan->format = srv[0];
    UINT forced=FM5ForcedSampleCount();
    if(forced==8 && plan->element_bytes==4)plan->expanded_value=0x76543210;
    /* R8 is retained solely as a typed legacy binding: eight indices cannot
       fit that format. Every exact guarded forced8 resolver reads all eight
       physical color samples directly and never reads this legacy binding.
       An unknown FMASK shader family is rejected by the selector above. */
    plan->fragments = plan->coverage_samples = forced==8?8:4;
    return S_OK;
}

HRESULT WINAPI FM5CreateLegacyPixelShader(void *native_device, const void *bytecode,
    SIZE_T bytes, void *class_linkage, void **out) {
    if (!native_device) return E_POINTER;
    void *method = (*(void ***)native_device)[15];
    CreatePixelShader create = (CreatePixelShader)method;
    const void *selected = bytecode;
    SIZE_T selected_bytes = bytes;
    HRESULT choice = FM5SelectFourSampleResolve(bytecode,bytes,FM5ForcedSampleCount(),
                                               &selected,&selected_bytes);
    if (FAILED(choice)) return choice;
    if (choice == S_OK) {
#ifndef FM5_MSAA4_TEST
        static const BYTE signature[16] = {0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56,0x41,0x57};
        const BYTE *host = (const BYTE *)GetModuleHandleW(NULL);
        if ((const BYTE *)method != host + 0x664c0 || memcmp(method,signature,sizeof(signature)))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
#endif
        static volatile LONG logged;
        if (!InterlockedExchange(&logged,1)) {
            char line[192]; DWORD written;
            int length = _snprintf_s(line,sizeof(line),_TRUNCATE,
                "{\"event\":\"fm5_forced_sample_resolve_selected\",\"original_bytes\":%llu,\"replacement_bytes\":%llu}\r\n",
                (unsigned long long)bytes,(unsigned long long)selected_bytes);
            if (length > 0) WriteFile(GetStdHandle(STD_ERROR_HANDLE),line,(DWORD)length,&written,NULL);
        }
    }
    HRESULT result=create(native_device,selected,selected_bytes,class_linkage,out);
    if(SUCCEEDED(result) && out && *out && pixel_shader_observer)
        pixel_shader_observer(*out,bytecode,bytes);
    return result;
}
