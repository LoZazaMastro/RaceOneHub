#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "fm5_native_loading.h"
#include <wincodec.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"msimg32.lib")
typedef struct Image {HBITMAP bitmap;UINT width,height;} Image;
static Image cached;static INIT_ONCE once=INIT_ONCE_STATIC_INIT;static LONG logged;
static volatile LONG warmup_thread;
BOOL FM5BeginOriginalPreparation(void){return InterlockedCompareExchange(&warmup_thread,(LONG)GetCurrentThreadId(),0)==0;}
void FM5EndOriginalPreparation(void){InterlockedCompareExchange(&warmup_thread,0,(LONG)GetCurrentThreadId());}
static HRESULT error(void){DWORD e=GetLastError();return e?HRESULT_FROM_WIN32(e):E_FAIL;}
BOOL FM5PreparationPercent(uint64_t done,uint64_t total,unsigned *out) {
    uint64_t remainder=0;unsigned i;
    if(!out)return FALSE;if(!total){*out=0;return FALSE;}
    if(done>=total){*out=100;return TRUE;}
    /* floor(100*done/total), avoiding both uint64 multiplication overflow and
       double rounding to100% before the final real work item completed. */
    *out=0;for(i=0;i<100;i++){if(remainder>=total-done){remainder-=total-done;(*out)++;}else remainder+=done;}
    return TRUE;
}
BOOL FM5PreparationSweep(int width,uint64_t milliseconds,int *left,int *right) {
    uint64_t phase=milliseconds%1800;int band;
    if(!left||!right||width<=0)return FALSE;
    if(phase>900)phase=1800-phase;
    band=width/5;if(band<1)band=1;
    *left=(int)(((uint64_t)(width-band)*phase)/900);*right=*left+band;return TRUE;
}
static HRESULT load(const wchar_t *path,Image *image) {
    IWICImagingFactory *f=NULL;IWICBitmapDecoder *d=NULL;IWICBitmapFrameDecode *frame=NULL;IWICFormatConverter *c=NULL;
    HRESULT hr,co;BITMAPINFO bi={0};void *bits;
    co=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);if(FAILED(co)&&co!=RPC_E_CHANGED_MODE)return co;
    hr=CoCreateInstance(&CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,&IID_IWICImagingFactory,(void**)&f);
    if(SUCCEEDED(hr))hr=IWICImagingFactory_CreateDecoderFromFilename(f,path,NULL,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&d);
    if(SUCCEEDED(hr))hr=IWICBitmapDecoder_GetFrame(d,0,&frame);
    if(SUCCEEDED(hr))hr=IWICBitmapFrameDecode_GetSize(frame,&image->width,&image->height);
    if(SUCCEEDED(hr)&&(!image->width||!image->height||image->width>16384||image->height>16384))hr=E_INVALIDARG;
    if(SUCCEEDED(hr))hr=IWICImagingFactory_CreateFormatConverter(f,&c);
    if(SUCCEEDED(hr))hr=IWICFormatConverter_Initialize(c,(IWICBitmapSource*)frame,&GUID_WICPixelFormat32bppBGR,WICBitmapDitherTypeNone,NULL,0,WICBitmapPaletteTypeCustom);
    if(SUCCEEDED(hr)){
        bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=(LONG)image->width;bi.bmiHeader.biHeight=-(LONG)image->height;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
        image->bitmap=CreateDIBSection(NULL,&bi,DIB_RGB_COLORS,&bits,NULL,0);
        hr=image->bitmap?IWICFormatConverter_CopyPixels(c,NULL,image->width*4,image->width*image->height*4,bits):error();
    }
    if(FAILED(hr)&&image->bitmap){DeleteObject(image->bitmap);image->bitmap=NULL;}
    if(c)IWICFormatConverter_Release(c);if(frame)IWICBitmapFrameDecode_Release(frame);if(d)IWICBitmapDecoder_Release(d);if(f)IWICImagingFactory_Release(f);
    if(SUCCEEDED(co))CoUninitialize();return hr;
}
static void release_cache(void){if(cached.bitmap){DeleteObject(cached.bitmap);cached.bitmap=NULL;}}
static BOOL CALLBACK init(PINIT_ONCE p,void *argument,void **context) {
    wchar_t path[32768],*slash;DWORD n;(void)p;(void)argument;(void)context;
    n=GetModuleFileNameW(NULL,path,32768);if(!n||n>=32768)return TRUE;slash=wcsrchr(path,L'\\');if(!slash)return TRUE;
    slash[1]=0;if(wcscat_s(path,32768,L"data\\ui\\fm5-background-clean.png"))return TRUE;
    if(SUCCEEDED(load(path,&cached)))atexit(release_cache);return TRUE;
}
static void text(HDC dc,const wchar_t *s,RECT r,int size,int weight,COLORREF color,UINT alignment) {
    HFONT font=CreateFontW(-size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    HGDIOBJ old=SelectObject(dc,font);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);
    DrawTextW(dc,s,-1,&r,DT_SINGLELINE|DT_VCENTER|alignment);SelectObject(dc,old);DeleteObject(font);
}
static HRESULT paint(HDC dc,const RECT *rect,const Image *image,uint64_t completed,uint64_t total,BOOL finished) {
    int w=rect->right-rect->left,h=rect->bottom-rect->top,x=(int)(w*.622),right=(int)(w*.945);double scale;
    HDC src;HGDIOBJ old;int sw,sh;BITMAPINFO bi={0};HBITMAP shade;void *pixels;RECT r;unsigned percent=0;
    BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};wchar_t status[96];HBRUSH brush;int saved;
    if(!dc||!image->bitmap||w<=0||h<=0)return E_INVALIDARG;
    if(total&&completed>total)completed=total;
    saved=SaveDC(dc);SetViewportOrgEx(dc,rect->left,rect->top,NULL);
    src=CreateCompatibleDC(dc);if(!src){RestoreDC(dc,saved);return error();}
    old=SelectObject(src,image->bitmap);scale=fmax((double)w/image->width,(double)h/image->height);sw=(int)(scale*image->width);sh=(int)(scale*image->height);
    SetStretchBltMode(dc,HALFTONE);StretchBlt(dc,(w-sw)/2,(h-sh)/2,sw,sh,src,0,0,(int)image->width,(int)image->height,SRCCOPY);SelectObject(src,old);
    bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-1;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    shade=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&pixels,NULL,0);
    if(shade){int k;for(k=0;k<w;k++){double t=(double)k/w,a=.08+.88/(1.+exp(-(t-.51)*17.));((DWORD*)pixels)[k]=(DWORD)(a*255)<<24;}
        old=SelectObject(src,shade);AlphaBlend(dc,0,0,w,h,src,0,0,w,1,blend);SelectObject(src,old);DeleteObject(shade);}
    DeleteDC(src);
    r=(RECT){x,(int)(h*.115),right,(int)(h*.15)};text(dc,L"Forza Motorsport 5",r,(int)(h*.015),FW_SEMIBOLD,RGB(255,138,68),DT_LEFT);
    r=(RECT){x,(int)(h*.151),right,(int)(h*.213)};text(dc,finished?L"Starting game":L"Preparing graphics",r,(int)(h*.035),FW_LIGHT,RGB(255,255,255),DT_LEFT);
    r=(RECT){x,(int)(h*.245),right,(int)(h*.249)};brush=CreateSolidBrush(RGB(60,62,67));FillRect(dc,&r,brush);DeleteObject(brush);
    if(total){double ratio=completed>=total?1.0:(double)completed/(double)total;r.right=x+(int)((right-x)*ratio);}
    else {int first,last;if(FM5PreparationSweep(right-x,GetTickCount64(),&first,&last)){r.left=x+first;r.right=x+last;}}
    brush=CreateSolidBrush(RGB(255,138,68));FillRect(dc,&r,brush);DeleteObject(brush);
    r=(RECT){x,(int)(h*.264),right,(int)(h*.3)};
    text(dc,L"Please wait",r,(int)(h*.018),FW_NORMAL,RGB(240,240,243),DT_LEFT);
    if(FM5PreparationPercent(completed,total,&percent)){swprintf_s(status,96,L"%u%%",percent);text(dc,status,r,(int)(h*.018),FW_SEMIBOLD,RGB(255,138,68),DT_RIGHT);}
    RestoreDC(dc,saved);return S_OK;
}
HRESULT FM5PaintPreparationImage(HDC dc,const RECT *rect,HBITMAP bitmap,UINT width,UINT height,uint64_t completed,uint64_t total){
    Image image={bitmap,width,height};if(!rect||!width||!height)return E_INVALIDARG;
    return paint(dc,rect,&image,completed,total,FALSE);
}
static BOOL host_valid(BYTE *host){
    static const BYTE bytes[16]={0x48,0x89,0x5c,0x24,0x20,0x55,0x56,0x57,0x48,0x83,0xec,0x60,0x48,0x8b,0x05,0xcd};
    IMAGE_DOS_HEADER *d=(IMAGE_DOS_HEADER*)host;IMAGE_NT_HEADERS64 *n;
    if(!host||d->e_magic!=IMAGE_DOS_SIGNATURE||d->e_lfanew<0||d->e_lfanew>4096)return FALSE;
    n=(IMAGE_NT_HEADERS64*)(host+d->e_lfanew);return n->Signature==IMAGE_NT_SIGNATURE&&n->OptionalHeader.SizeOfImage>=0x37d010&&!memcmp(host+0x4a060,bytes,16);
}
typedef struct NativePaintOverlay {void **vtable;void *original;} NativePaintOverlay;
typedef void (*NativeWindowPaint)(HWND,void *);
static void paint_native_overlay(void *self,HDC *dc,const RECT *rect){
    NativePaintOverlay *overlay=(NativePaintOverlay*)self;
    BYTE *host=(BYTE*)GetModuleHandleW(NULL),local[24]={0};const BYTE *snapshot;
    void *original=*(void**)((BYTE*)overlay->original+0x38);uint64_t done,total;
    /* This callback can update native model/tasks. Always execute it exactly
       once on the native offscreen DC, before covering its original pixels. */
    if(original)((void(*)(void*,HDC*,const RECT*))(*(void***)original)[2])(original,dc,rect);
    if(!host_valid(host)||!dc||!rect)return;
    snapshot=((void*(*)(void*))(host+0x57f80))(local);if(!snapshot)return;
    memcpy(&done,snapshot+8,8);memcpy(&total,snapshot+16,8);
    InitOnceExecuteOnce(&once,init,NULL,NULL);
    if(!cached.bitmap||FAILED(paint(*dc,rect,&cached,done,total,FALSE)))return;
    if(InterlockedCompareExchange(&logged,1,0)==0){fprintf(stderr,"FM5_NATIVE_LOADING completed=%llu total=%llu phase=original-preparation\n",(unsigned long long)done,(unsigned long long)total);fflush(stderr);}
}
static void compose_native_paint(HWND window,void *native_callback,NativeWindowPaint native_paint){
    void *vtable[3]={NULL,NULL,(void*)paint_native_overlay};
    NativePaintOverlay closure={vtable,native_callback};
    BYTE borrowed_function[0x40]={0};
    /*44bb0 borrows std::function+38 only; it neither copies nor destroys it. */
    *(void**)(borrowed_function+0x38)=&closure;
    native_paint(window,borrowed_function);
}
#ifdef FM5_NATIVE_UI_TEST
void FM5TestComposeNativePainter(HWND window,void *native_callback,void(*native_paint)(HWND,void*)){
    compose_native_paint(window,native_callback,native_paint);
}
#endif
__declspec(dllexport) void FM5PaintOriginalPreparation(HWND window,void *native_callback){
    BYTE *host=(BYTE*)GetModuleHandleW(NULL);
    static const BYTE guard[16]={0x56,0x41,0x56,0x48,0x81,0xec,0x88,0,0,0,0x48,0x8b,0x05,0x7e,0x36,0x2d};
    if(!host_valid(host)||memcmp(host+0x44bb0,guard,sizeof(guard)))return;
    if((DWORD)InterlockedCompareExchange(&warmup_thread,0,0)!=GetCurrentThreadId()||
       window!=*(HWND*)(host+0x32b5b0)||!native_callback||!*(void**)((BYTE*)native_callback+0x38)){
        ((NativeWindowPaint)(host+0x44bb0))(window,native_callback);return;
    }
    compose_native_paint(window,native_callback,(NativeWindowPaint)(host+0x44bb0));
}
__declspec(dllexport) void FM5DrawPreparation(void *closure,HDC *dc,const RECT *rect) {
    BYTE *host=(BYTE*)GetModuleHandleW(NULL);const BYTE *snapshot;uint64_t done,total;
    if(!host_valid(host)||!closure||!dc||!rect)return;
    if(*(uintptr_t*)closure!=(uintptr_t)(host+0x29d970))goto fallback;
    snapshot=*(const BYTE**)((BYTE*)closure+8);if(!snapshot)goto fallback;
    memcpy(&done,snapshot+8,8);memcpy(&total,snapshot+16,8);InitOnceExecuteOnce(&once,init,NULL,NULL);
    if(!cached.bitmap||FAILED(paint(*dc,rect,&cached,done,total,FALSE)))goto fallback;
    if(InterlockedCompareExchange(&logged,1,0)==0){fprintf(stderr,"FM5_NATIVE_LOADING completed=%llu total=%llu phase=graphics\n",(unsigned long long)done,(unsigned long long)total);fflush(stderr);}return;
fallback:((void(*)(void *,HDC *,const RECT *))(host+0x4a060))(closure,dc,rect);
}
__declspec(dllexport) void FM5DrawPreparationComplete(void *closure,HDC *dc,const RECT *rect) {
    BYTE *host=(BYTE*)GetModuleHandleW(NULL);
    if(!host_valid(host)||!closure||!dc||!rect)return;
    InitOnceExecuteOnce(&once,init,NULL,NULL);
    if(*(uintptr_t*)closure==(uintptr_t)(host+0x29d938)&&cached.bitmap&&SUCCEEDED(paint(*dc,rect,&cached,1,1,TRUE)))return;
    ((void(*)(void *,HDC *,const RECT *))(host+0x4a000))(closure,dc,rect);
}
HRESULT FM5RenderLoadingPreview(const wchar_t *asset,const wchar_t *output,uint64_t completed,uint64_t total) {
    Image image={0};HRESULT hr=load(asset,&image);HDC dc,mem;BITMAPINFO bi={0};HBITMAP bm;HGDIOBJ old;void *bits;RECT r={0,0,1920,1080};HANDLE f;DWORD wrote;BITMAPFILEHEADER fh={0};
    if(FAILED(hr))return hr;bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=1920;bi.bmiHeader.biHeight=-1080;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    dc=GetDC(NULL);mem=CreateCompatibleDC(dc);bm=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,NULL,0);ReleaseDC(NULL,dc);
    if(!bm){DeleteDC(mem);DeleteObject(image.bitmap);return error();}old=SelectObject(mem,bm);hr=paint(mem,&r,&image,completed,total,FALSE);
    f=CreateFileW(output,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);if(f==INVALID_HANDLE_VALUE)hr=error();else{
        fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+1920*1080*4;
        if(!WriteFile(f,&fh,sizeof(fh),&wrote,NULL)||!WriteFile(f,&bi.bmiHeader,sizeof(BITMAPINFOHEADER),&wrote,NULL)||!WriteFile(f,bits,1920*1080*4,&wrote,NULL))hr=E_FAIL;CloseHandle(f);}
    SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);DeleteObject(image.bitmap);return hr;
}
