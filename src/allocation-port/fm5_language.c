#include "fm5_language.h"
#include <stdint.h>
#include <string.h>
#include <wchar.h>
#include "fm5_language_guards.inc"

typedef struct TextLanguage { const wchar_t *locale; const char *prefix; UINT index, rva; } TextLanguage;
static const TextLanguage languages[] = {
    {L"en-US","EN",0,0x1cbf064}, {L"en-GB","GB",2,0x1cbf060},
    {L"ja-JP","JP",3,0x1cbf06c}, {L"de-DE","DE",4,0x1cbf068},
    {L"fr-FR","FR",5,0x1cbf054}, {L"es-MX","MX",6,0x1cbf050},
    {L"es-ES","ES",7,0x1cbf05c}, {L"it-IT","IT",8,0x1cbf058},
    {L"ko-KR","KO",9,0x1cbf044}, {L"zh-TW","CHT",10,0x1cbf040},
    {L"pt-BR","BR",11,0x1cbf038}, {L"cs-CZ","CZ",12,0x1cbf04c},
    {L"hu-HU","HU",13,0x1cbf048}, {L"pl-PL","PL",14,0x1cbf034},
    {L"ru-RU","RU",15,0x1cbf030}, {L"nl-NL","NL",16,0x1cbf03c}
};

static BOOL readable(const void *pointer,SIZE_T bytes) {
    MEMORY_BASIC_INFORMATION info;
    if(!pointer || !bytes || !VirtualQuery(pointer,&info,sizeof(info)) ||
       info.State!=MEM_COMMIT || (info.Protect&(PAGE_NOACCESS|PAGE_GUARD)))return FALSE;
    uintptr_t begin=(uintptr_t)pointer,region=(uintptr_t)info.BaseAddress;
    return begin>=region && begin-region<=info.RegionSize && bytes<=info.RegionSize-(begin-region);
}

static HRESULT validate_host(const BYTE *host,SIZE_T host_bytes) {
    if(!host || host_bytes<0x31a8d0)return E_INVALIDARG;
    const IMAGE_DOS_HEADER *dos=(const IMAGE_DOS_HEADER*)host;
    if(dos->e_magic!=IMAGE_DOS_SIGNATURE || dos->e_lfanew<=0 || dos->e_lfanew>4096)
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    const IMAGE_NT_HEADERS64 *pe=(const IMAGE_NT_HEADERS64*)(host+dos->e_lfanew);
    if(pe->Signature!=IMAGE_NT_SIGNATURE || pe->OptionalHeader.Magic!=IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
       pe->OptionalHeader.SizeOfImage!=host_bytes ||
       memcmp(host+0x37d00,guard_host_locale,sizeof(guard_host_locale)) ||
       memcmp(host+0x3abf0,guard_host_set_locale,sizeof(guard_host_set_locale)))
        return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    return S_OK;
}

static HRESULT validate_guest(BYTE *guest,SIZE_T guest_bytes) {
    if(!guest || guest_bytes<0x25d2945)return E_INVALIDARG;
    for(UINT i=0;i<_countof(guest_guards);++i) {
        const LanguageGuard *g=&guest_guards[i];
        if(g->rva>guest_bytes || g->bytes>guest_bytes-g->rva ||
           memcmp(guest+g->rva,g->original,g->bytes))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    }
    /* Settings constructor proves an eight-byte field at+248d; next+2495.
       Changing its source LEA preserves its real later strcpy_s call. */
    for(UINT i=0;i<_countof(languages);++i) {
        SIZE_T bytes=strlen(languages[i].prefix)+1;
        if(memcmp(guest+languages[i].rva,languages[i].prefix,bytes))
            return HRESULT_FROM_WIN32(ERROR_BAD_EXE_FORMAT);
    }
    return S_OK;
}

static HRESULT match_locale(const wchar_t *selected,SIZE_T length,const TextLanguage **language) {
    *language=NULL;
    if(!length)return S_FALSE;
    if(length>85 || !readable(selected,(length+1)*sizeof(wchar_t)) || selected[length])return E_INVALIDARG;
    if(length==6 && CompareStringOrdinal(selected,6,L"system",6,TRUE)==CSTR_EQUAL)return S_FALSE;
    /* The original Azimuth catalog names Traditional Chinese zh-HK.
       FM5 maps both zh-HK and canonical zh-TW to the same CHT enum10. */
    if(length==5 && CompareStringOrdinal(selected,5,L"zh-HK",5,TRUE)==CSTR_EQUAL)
        selected=L"zh-TW";
    for(UINT i=0;i<_countof(languages);++i)
        if(length==wcslen(languages[i].locale) &&
           CompareStringOrdinal(selected,(int)length,languages[i].locale,(int)length,TRUE)==CSTR_EQUAL) {
            *language=&languages[i];return S_OK;
        }
    return HRESULT_FROM_WIN32(ERROR_NOT_SUPPORTED);
}

static HRESULT read_host_locale(const BYTE *host,const TextLanguage **language) {
    *language=NULL;
    SIZE_T length,capacity;
    memcpy(&length,host+0x31a8c0,sizeof(length));
    memcpy(&capacity,host+0x31a8c8,sizeof(capacity));
    if(!length)return S_FALSE;
    if(length>85 || capacity<length || capacity<7)return E_INVALIDARG;
    const wchar_t *selected=(const wchar_t*)(host+0x31a8b0);
    if(capacity>7)memcpy(&selected,host+0x31a8b0,sizeof(selected));
    return match_locale(selected,length,language);
}

static void make_plan(BYTE *guest,const TextLanguage *language,FM5TextLanguagePlan *plan) {
    plan->site=guest+0x5d1b38;
    memcpy(plan->replacement,plan->site,sizeof(plan->replacement));
    int32_t displacement=(int32_t)language->rva-(int32_t)0x5d1b3f;
    memcpy(plan->replacement+3,&displacement,sizeof(displacement));
    memcpy(plan->prefix,language->prefix,strlen(language->prefix)+1);
    plan->language_index=language->index;
}

const wchar_t *FM5TextLanguageLocaleAt(UINT ordinal) {
    return ordinal<_countof(languages)?languages[ordinal].locale:NULL;
}

HRESULT FM5ReadHostSelectedLocaleIndex(const BYTE *host,SIZE_T host_bytes,UINT *index) {
    if(!index)return E_POINTER;
    *index=0;
    HRESULT result=validate_host(host,host_bytes);
    if(FAILED(result))return result;
    const TextLanguage *language=NULL;
    result=read_host_locale(host,&language);
    if(result==S_OK)*index=language->index;
    return result;
}

HRESULT FM5PrepareTextLanguageForLocale(const BYTE *host,SIZE_T host_bytes,
    BYTE *guest,SIZE_T guest_bytes,const wchar_t *locale,SIZE_T length,FM5TextLanguagePlan *plan) {
    if(!plan)return E_POINTER;
    memset(plan,0,sizeof(*plan));
    HRESULT result=validate_host(host,host_bytes);
    if(FAILED(result))return result;
    result=validate_guest(guest,guest_bytes);
    if(FAILED(result))return result;
    const TextLanguage *language=NULL;
    result=match_locale(locale,length,&language);
    if(result!=S_OK)return result;
    make_plan(guest,language,plan);
    return S_OK;
}

HRESULT FM5PrepareTextLanguage(const BYTE *host,SIZE_T host_bytes,
    BYTE *guest,SIZE_T guest_bytes,FM5TextLanguagePlan *plan) {
    if(!plan)return E_POINTER;
    memset(plan,0,sizeof(*plan));
    HRESULT result=validate_host(host,host_bytes);
    if(FAILED(result))return result;
    result=validate_guest(guest,guest_bytes);
    if(FAILED(result))return result;
    const TextLanguage *language=NULL;
    result=read_host_locale(host,&language);
    if(result!=S_OK)return result;
    make_plan(guest,language,plan);
    return S_OK;
}

void FM5CommitTextLanguage(const FM5TextLanguagePlan *plan) {
    if(plan && plan->site)memcpy(plan->site,plan->replacement,sizeof(plan->replacement));
}
