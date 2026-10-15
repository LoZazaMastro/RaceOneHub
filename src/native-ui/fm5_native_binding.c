#define WIN32_LEAN_AND_MEAN
#include "fm5_native_ui.h"
#include "fm5_native_loading.h"
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "fm5_ui_guards.h"

/* Original 0.9.9 Settings is a native 0x100-byte C++ aggregate. Its strings,
   vectors and allocators are never synthesized by this module. */
typedef struct NativeString { union { wchar_t small[8]; wchar_t *heap; } data; size_t size,capacity; } NativeString;
typedef struct Binding { BYTE *host,*settings;void *path;FM5UIModel model;unsigned initial_graphics;
    BOOL log_preparation;unsigned logged,previous_device,previous_ready;
    uint64_t previous_completed,previous_total;ULONGLONG last_log;
} Binding;
static const wchar_t *locales[]={L"",L"en-US",L"en-GB",L"zh-TW",L"cs-CZ",L"de-DE",L"es-ES",L"es-MX",L"fr-FR",L"hu-HU",L"it-IT",L"ja-JP",L"ko-KR",L"nl-NL",L"pl-PL",L"pt-BR",L"ru-RU"};
static const wchar_t *labels[]={L"",L"English (US)",L"English (UK)",L"Chinese (Traditional)",L"Czech",L"German",L"Spanish (Spain)",L"Spanish (Mexico)",L"French",L"Hungarian",L"Italian",L"Japanese",L"Korean",L"Dutch",L"Polish",L"Portuguese (Brazil)",L"Russian"};
static const wchar_t *prefixes[]={L"",L"EN",L"GB",L"CHT",L"CZ",L"DE",L"ES",L"MX",L"FR",L"HU",L"IT",L"JP",L"KO",L"NL",L"PL",L"BR",L"RU"};
static const wchar_t *str(const NativeString *s){return s->capacity>7?s->data.heap:s->data.small;}
static BOOL preparation(void *context,uint64_t *completed,uint64_t *total){
    Binding *b=context;BYTE local[24]={0};const BYTE *snapshot;
    snapshot=((void*(*)(void*))(b->host+0x57f80))(local);
    if(!snapshot){*completed=0;*total=0;return FALSE;}
    memcpy(completed,snapshot+8,8);memcpy(total,snapshot+16,8);
    if(b->log_preparation&&b->logged<8){
        unsigned device=b->host[0x32b598],ready=snapshot[0];ULONGLONG now=GetTickCount64();
        if(!b->logged||device!=b->previous_device||ready!=b->previous_ready||
           ((now-b->last_log>=1000)&&(*completed!=b->previous_completed||*total!=b->previous_total))){
            fprintf(stderr,"FM5_NATIVE_UI_PREPARATION device_ready=%u native_ready=%u completed=%llu total=%llu backend_hresult=%08lx\n",device,ready,(unsigned long long)*completed,(unsigned long long)*total,*(unsigned long*)(b->host+0x32b588));fflush(stderr);
            b->previous_device=device;b->previous_ready=ready;b->previous_completed=*completed;b->previous_total=*total;b->last_log=now;b->logged++;
        }
    }
    return snapshot[0]!=0;
}
static unsigned value(FM5UIModel *m,unsigned row){return m->rows[row].choices[m->rows[row].selected].value;}
static void add(Binding *b,const wchar_t *label,const wchar_t **names,const unsigned *values,unsigned n,unsigned current) {
    FM5UIRow *r=&b->model.rows[b->model.count++];unsigned i;r->label=label;r->enabled=TRUE;r->count=n;
    for(i=0;i<n;i++){r->choices[i]=(FM5UIChoice){names[i],values[i]};if(values[i]==current)r->selected=i;}
}
static void add_antialiasing(Binding *b,unsigned current) {
    static const wchar_t *names[]={L"1\x00d7",L"2\x00d7",L"4\x00d7"};
    static const unsigned samples[]={1,2,4};
    add(b,L"MSAA",names,samples,3,current==8?4:current);
}
static void add_resolutions(Binding *b,unsigned current) {
    static const wchar_t *names[]={L"1080p (1920x1080)",L"1440p (2560x1440)",L"4K (3840x2160)",L"5K (5120x2880)",L"6K (5760x3240)",L"7K (6720x3780)",L"8K (7680x4320)"};
    static const unsigned heights[]={1080,1440,2160,2880,3240,3780,4320};
    add(b,L"Resolution",names,heights,(unsigned)(sizeof(heights)/sizeof(heights[0])),current);
}
static HRESULT commit(FM5UIModel *model,void *context) {
    Binding *b=context;BYTE copy[0x100];unsigned i;BOOL alive=FALSE;
    typedef void *(*Clone)(void *,const void *);typedef void (*Destroy)(void *);
    typedef void *(*Move)(void *,void *);typedef void *(*Assign)(void *,const wchar_t *,size_t);
    typedef void (*Save)(void *,void *,unsigned);typedef void *(*Preset)(void *,unsigned);
    HRESULT hr=E_FAIL;
    if(value(model,3)!=1&&value(model,3)!=2&&value(model,3)!=4)return E_INVALIDARG;
    if(model->preparation){uint64_t completed,total;if(!model->preparation(context,&completed,&total))return E_PENDING;}
    __try {
        ((Clone)(b->host+0x13ea0))(copy,b->settings);alive=TRUE;
        *(unsigned*)(copy+0)=value(model,0);*(unsigned*)(copy+8)=value(model,1);
        *(unsigned*)(copy+4)=value(model,3);copy[0xd]=(BYTE)value(model,4);copy[0xe]=(BYTE)value(model,5);
        /* Above 30 FPS the original launcher always selects real-time pacing. */
        if(*(unsigned*)(copy+8)>30)copy[0xc]=1;
        if(model->rows[2].selected!=b->initial_graphics)
            ((Preset)(b->host+0x31e0))(copy+0xd0,value(model,2));
        i=value(model,6);((Assign)(b->host+0x7bd0))(copy+0x18,locales[i],wcslen(locales[i]));
        /* Native INI parser7f3c0 retains the literal system sentinel. */
        ((Assign)(b->host+0x7bd0))(copy+0x38,L"system",6);
        *(unsigned*)(copy+0xe8)=value(model,7);
        /* Genuine native per-field writer retains unrelated registrations and content. */
        {static const unsigned fields[]={1,2,3,4,5,6,7,8,9,0x10};
        for(i=0;i<sizeof(fields)/sizeof(fields[0]);i++)((Save)(b->host+0x7f6f0))(b->path,copy,fields[i]);}
        ((Move)(b->host+0x6f00))(b->settings,copy);hr=S_OK;
    } __finally {if(alive)((Destroy)(b->host+0x6eb0))(copy);}
    return hr;
}
static BOOL guards(BYTE *host) {
    unsigned i;const IMAGE_DOS_HEADER *dos=(const IMAGE_DOS_HEADER*)host;const IMAGE_NT_HEADERS64 *nt;
    if(!host||dos->e_magic!=IMAGE_DOS_SIGNATURE||dos->e_lfanew<0||dos->e_lfanew>0x1000)return FALSE;
    nt=(const IMAGE_NT_HEADERS64*)(host+dos->e_lfanew);
    if(nt->Signature!=IMAGE_NT_SIGNATURE||nt->OptionalHeader.SizeOfImage<0x37d010)return FALSE;
    for(i=0;i<FM5_UI_GUARD_COUNT;i++)if(memcmp(host+fm5_ui_guards[i].rva,fm5_ui_guards[i].bytes,16))return FALSE;
    return TRUE;
}
static void populate_game_folder(Binding *b,void *folder,const NativeString *resolved) {
    /* Original selector52bc..52d9: GameFolder.path at+0 followed by the
       genuine0x1a0-byte parsed Game at+20. Reuse the original manifest/hash
       parser and move assignment, including their error/ownership behavior. */
    BYTE parsed[0x1a0];BOOL alive=FALSE;uintptr_t profiles[2]={(uintptr_t)(b->host+0x29a1c0),2};
    typedef void *(*Assign)(void *,const wchar_t *,size_t);
    typedef void *(*Parse)(void *,void *,const uintptr_t *);
    typedef void *(*Move)(void *,void *);typedef void (*Destroy)(void *);
    __try {
        ((Assign)(b->host+0x7bd0))(folder,str(resolved),resolved->size);
        ((Parse)(b->host+0x875b0))(parsed,folder,profiles);alive=TRUE;
        ((Move)(b->host+0x7140))((BYTE*)folder+0x20,parsed);
    } __finally {if(alive)((Destroy)(b->host+0x7070))(parsed);}
}
static HRESULT prepare(Binding *b,void *game) {
    static const wchar_t *fps[]={L"30 FPS",L"60 FPS",L"120 FPS"};static const unsigned rates[]={30,60,120};
    static const wchar_t *presets[]={L"Low",L"Original",L"High",L"Ultra",L"Custom"};static const unsigned levels[]={0,1,2,3,4};
    static const wchar_t *bools[]={L"Off",L"On"};static const unsigned boolvalues[]={0,1};
    static const wchar_t *display[]={L"Windowed",L"Fullscreen"};
    static const wchar_t *audio[]={L"System",L"Stereo",L"Multichannel"};static const unsigned channels[]={0,1,2};
    BYTE *s=b->settings;unsigned i,preset=4;unsigned mapped[6];NativeString resolved={0};
    uintptr_t begin=*(uintptr_t*)(s+0x58),end=*(uintptr_t*)(s+0x60),entry;
    NativeString *language=(NativeString*)(s+0x18);FM5UIRow *r;wchar_t file[32768];
    typedef void *(*Preset)(void *,unsigned);typedef void *(*Resolve)(void *,void *,void *);typedef void (*StringDestroy)(void *);
    if(!begin||end<begin||(end-begin)%0x78||end-begin>0x7800)return E_INVALIDARG;
    for(entry=begin;entry<end;entry+=0x78)if(!_wcsicmp(str((NativeString*)entry),str((NativeString*)(s+0x70))))break;
    if(entry==end)return HRESULT_FROM_WIN32(ERROR_NOT_FOUND);
    for(i=0;i<4;i++){((Preset)(b->host+0x31e0))(mapped,i);if(!memcmp(mapped,s+0xd0,sizeof(mapped)))preset=i;}
    add_resolutions(b,*(unsigned*)s);
    add(b,L"Frame rate",fps,rates,3,*(unsigned*)(s+8));
    add(b,L"Graphics",presets,levels,preset==4?5:4,preset);b->initial_graphics=b->model.rows[2].selected;
    add_antialiasing(b,*(unsigned*)(s+4));add(b,L"VSync",bools,boolvalues,2,s[0xd]);
    add(b,L"Display",display,boolvalues,2,s[0xe]);
    r=&b->model.rows[b->model.count++];r->label=L"Language";r->enabled=TRUE;
    __try {
        ((Resolve)(b->host+0x74660))(&resolved,b->path,(void*)(entry+0x20));
        if(!((NativeString*)game)->size)populate_game_folder(b,game,&resolved);
        for(i=1;i<17;i++){
            if(swprintf_s(file,32768,L"%ls\\Mount\\media\\stringtables\\%ls.zip",str(&resolved),prefixes[i])<0)continue;
            if(GetFileAttributesW(file)!=INVALID_FILE_ATTRIBUTES){r->choices[r->count]=(FM5UIChoice){labels[i],i};if(!_wcsicmp(str(language),locales[i]))r->selected=r->count;r->count++;}
        }
    } __finally {((StringDestroy)(b->host+0x8d50))(&resolved);}
    if(!r->count)return HRESULT_FROM_WIN32(ERROR_RESOURCE_LANG_NOT_FOUND);
    add(b,L"Audio output",audio,channels,3,*(unsigned*)(s+0xe8));
    b->model.focused=b->model.count;b->model.commit=commit;b->model.preparation=preparation;b->model.context=b;return S_OK;
}
/* Exactly the native182a0 five-argument call ABI. Root redirects only5e13.
   Both ordinary no-argument starts and explicit --game starts populate the
   real GameFolder through the same native parser before accepting Start. */
__declspec(dllexport) unsigned char FM5RunPreGame(void *settings,void *fixed,void *path,void *game,unsigned char skip_requested) {
    Binding b={0};wchar_t asset[32768],startup[32768];wchar_t *slash;HWND hwnd;HRESULT hr;unsigned char original_display;
    typedef HWND (*Window)(void);typedef void (*Fullscreen)(unsigned char);
    typedef unsigned char (*NativeMenu)(void *,void *,void *,void *,unsigned char);
    b.host=(BYTE*)GetModuleHandleW(NULL);b.settings=settings;b.path=path;
    if(!guards(b.host)||!settings||!path||!game){fprintf(stderr,"FM5_NATIVE_UI guard or arguments rejected\n");return 0;}
    b.log_preparation=TRUE;
    /* Fifth argument is the native automatic-Start mode. Preserve its genuine
       message-pumping preparation/start lifecycle for explicit CLI skips. */
    if(skip_requested)return ((NativeMenu)(b.host+0x182a0))(settings,fixed,path,game,1);
    if(!GetModuleFileNameW(NULL,startup,32768))return 0;slash=wcsrchr(startup,L'\\');if(!slash)return 0;slash[1]=0;
    if(swprintf_s(asset,32768,L"%lsdata\\ui\\fm5-background-clean.png",startup)<0)return 0;
    if(wcscat_s(startup,32768,L"forzamastro.ini"))return 0;
    if(!GetPrivateProfileIntW(L"startup",L"show_pre_game",1,startup)){
        return ((NativeMenu)(b.host+0x182a0))(settings,fixed,path,game,1);
    }
    /* Execute the complete proven native automatic-Start state machine first.
       Its return stops here, before selector/game startup. Only paint callbacks
       are reskinned while it owns this thread; original model/task ownership,
       parsing, normalization and preparation are left intact. */
    if(!FM5BeginOriginalPreparation())return 0;
    {unsigned char accepted=0,saved_display=b.settings[0xe];
    __try {
        b.settings[0xe]=1; /* Every pre-game phase is fullscreen. */
        accepted=((NativeMenu)(b.host+0x182a0))(settings,fixed,path,game,1);
    } __finally {b.settings[0xe]=saved_display;FM5EndOriginalPreparation();}
    if(!accepted)return 0;}
    hr=prepare(&b,game);if(FAILED(hr)){fprintf(stderr,"FM5_NATIVE_UI model failed HRESULT=%08lx\n",(unsigned long)hr);return 0;}
    hwnd=((Window)(b.host+0x6d1d0))();if(!hwnd)return 0;
    original_display=b.settings[0xe];((Fullscreen)(b.host+0x6d560))(1);
    hr=FM5RunNativePreGame(hwnd,&b.model,asset);
    ((Fullscreen)(b.host+0x6d560))(hr==S_OK?b.settings[0xe]:original_display);
    if(hr==S_OK||hr==S_FALSE){
        /* Native182a0's actual Start/Quit exit resets queued menu events and
           requests size/visibility/enabled/focus events on FrameworkView Activate. */
        *(uintptr_t*)(b.host+0x37cf80)=*(uintptr_t*)(b.host+0x37cf78);
        b.host[0x32b5da]=1;
        ((uintptr_t(*)(unsigned,uintptr_t,uintptr_t,uintptr_t))(b.host+0x1538f0))(8,0,0,0);
    }
    /* Selector59eb already spawned the genuine preparation worker. The stock
       menu prevents an early Start; our Start instead keeps the stock measured
       preparation wait before returning to guest startup. Never set its ready flag. */
    if(hr==S_OK)((void(*)(void))(b.host+0x49b80))();
    fprintf(stderr,"FM5_NATIVE_UI result=%08lx hwnd=%p process=%lu\n",(unsigned long)hr,hwnd,GetCurrentProcessId());
    return hr==S_OK;
}
