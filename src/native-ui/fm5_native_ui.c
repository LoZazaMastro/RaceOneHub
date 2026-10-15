#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include "fm5_native_ui.h"
#include "fm5_native_loading.h"
#include <wincodec.h>
#include <xinput.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>
#pragma comment(lib,"windowscodecs.lib")
#pragma comment(lib,"ole32.lib")
#pragma comment(lib,"gdi32.lib")
#pragma comment(lib,"user32.lib")
#pragma comment(lib,"msimg32.lib")
#pragma comment(lib,"xinput.lib")

typedef struct UI {
    HWND hwnd; WNDPROC previous; FM5UIModel *model;
    HBITMAP background; UINT bw,bh; BOOL finished,accepted,pending_start;
    UINT_PTR timer; WORD buttons[4]; ULONGLONG repeats[4];
    RECT rows[FM5_UI_MAX_ROWS+2];
} UI;
static const wchar_t property[]=L"ForzaMastro5.NativePreGame.State.v1";
static const COLORREF accent=RGB(255,138,68);
static HRESULT winerror(void){DWORD e=GetLastError();return e?HRESULT_FROM_WIN32(e):E_FAIL;}

BOOL FM5NativeUIAction(FM5UIModel *m,unsigned action) {
    FM5UIRow *r;
    if(!m || !m->count || m->count>FM5_UI_MAX_ROWS || m->focused>m->count+1) return FALSE;
    if(action==1 || action==2) {
        unsigned position=m->focused==m->count?0:m->focused==m->count+1?m->count+1:m->focused+1;
        position=(position+m->count+2+(action==1?-1:1))%(m->count+2);
        m->focused=position==0?m->count:position==m->count+1?m->count+1:position-1;
    }
    else if((action==3||action==4) && m->focused<m->count) {
        r=&m->rows[m->focused];
        if(!r->enabled||!r->count||r->count>FM5_UI_MAX_CHOICES)return FALSE;
        r->selected=(r->selected+r->count+(action==3?-1:1))%r->count;
    } else return FALSE;
    m->error[0]=0; return TRUE;
}
static HRESULT image_load(const wchar_t *path,HBITMAP *out,UINT *w,UINT *h) {
    IWICImagingFactory *f=NULL; IWICBitmapDecoder *d=NULL;
    IWICBitmapFrameDecode *frame=NULL; IWICFormatConverter *c=NULL;
    BITMAPINFO b={0}; void *bits=NULL; HRESULT hr;
    *out=NULL;
    hr=CoCreateInstance(&CLSID_WICImagingFactory,NULL,CLSCTX_INPROC_SERVER,&IID_IWICImagingFactory,(void**)&f);
    if(SUCCEEDED(hr))hr=IWICImagingFactory_CreateDecoderFromFilename(f,path,NULL,GENERIC_READ,WICDecodeMetadataCacheOnLoad,&d);
    if(SUCCEEDED(hr))hr=IWICBitmapDecoder_GetFrame(d,0,&frame);
    if(SUCCEEDED(hr))hr=IWICBitmapFrameDecode_GetSize(frame,w,h);
    if(SUCCEEDED(hr)&&(!*w||!*h||*w>16384||*h>16384))hr=E_INVALIDARG;
    if(SUCCEEDED(hr))hr=IWICImagingFactory_CreateFormatConverter(f,&c);
    if(SUCCEEDED(hr))hr=IWICFormatConverter_Initialize(c,(IWICBitmapSource*)frame,&GUID_WICPixelFormat32bppBGR,WICBitmapDitherTypeNone,NULL,0,WICBitmapPaletteTypeCustom);
    if(SUCCEEDED(hr)) {
        b.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);b.bmiHeader.biWidth=(LONG)*w;
        b.bmiHeader.biHeight=-(LONG)*h;b.bmiHeader.biPlanes=1;b.bmiHeader.biBitCount=32;b.bmiHeader.biCompression=BI_RGB;
        *out=CreateDIBSection(NULL,&b,DIB_RGB_COLORS,&bits,NULL,0);
        if(!*out)hr=winerror();
        else hr=IWICFormatConverter_CopyPixels(c,NULL,*w*4,*w**h*4,(BYTE*)bits);
    }
    if(FAILED(hr)&&*out){DeleteObject(*out);*out=NULL;}
    if(c)IWICFormatConverter_Release(c);if(frame)IWICBitmapFrameDecode_Release(frame);
    if(d)IWICBitmapDecoder_Release(d);if(f)IWICImagingFactory_Release(f);return hr;
}
static void text(HDC dc,const wchar_t *s,RECT rect,int size,int weight,COLORREF color,UINT align) {
    HFONT font=CreateFontW(-size,0,0,0,weight,FALSE,FALSE,FALSE,DEFAULT_CHARSET,OUT_DEFAULT_PRECIS,CLIP_DEFAULT_PRECIS,CLEARTYPE_QUALITY,DEFAULT_PITCH,L"Segoe UI");
    HGDIOBJ old=SelectObject(dc,font);SetTextColor(dc,color);SetBkMode(dc,TRANSPARENT);
    DrawTextW(dc,s,-1,&rect,DT_SINGLELINE|DT_VCENTER|DT_END_ELLIPSIS|align);
    SelectObject(dc,old);DeleteObject(font);
}
static void line(HDC dc,int x,int y,int right,COLORREF color) {
    HPEN p=CreatePen(PS_SOLID,1,color);HGDIOBJ old=SelectObject(dc,p);
    MoveToEx(dc,x,y,NULL);LineTo(dc,right,y);SelectObject(dc,old);DeleteObject(p);
}
static void render(UI *u,HDC dc,int w,int h) {
    if(u->pending_start&&u->model->preparation){
        uint64_t completed=0,total=0;RECT whole={0,0,w,h};
        u->model->preparation(u->model->context,&completed,&total);
        if(SUCCEEDED(FM5PaintPreparationImage(dc,&whole,u->background,u->bw,u->bh,completed,total)))return;
    }
    HDC src=CreateCompatibleDC(dc); HGDIOBJ old=SelectObject(src,u->background);
    double scale=fmax((double)w/u->bw,(double)h/u->bh);
    int sw=(int)(u->bw*scale),sh=(int)(u->bh*scale),x=(int)(w*.622),right=(int)(w*.945);
    int top=(int)(h*.245),step=(int)(h*fmin(.052,.60/(u->model->count+2))),fs=(int)(h*.020); unsigned i;
    BITMAPINFO bi={0};void *pixels;HBITMAP shade;BLENDFUNCTION blend={AC_SRC_OVER,0,255,AC_SRC_ALPHA};RECT r;
    SetStretchBltMode(dc,HALFTONE);StretchBlt(dc,(w-sw)/2,(h-sh)/2,sw,sh,src,0,0,(int)u->bw,(int)u->bh,SRCCOPY);
    SelectObject(src,old);
    bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=w;bi.bmiHeader.biHeight=-1;
    bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    shade=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&pixels,NULL,0);
    if(shade){int k;for(k=0;k<w;k++){double t=(double)k/w;double a=.08+.88/(1.+exp(-(t-.51)*17.));((DWORD*)pixels)[k]=(DWORD)(a*255)<<24;}
        old=SelectObject(src,shade);AlphaBlend(dc,0,0,w,h,src,0,0,w,1,blend);SelectObject(src,old);DeleteObject(shade);}
    DeleteDC(src);
    r=(RECT){x,(int)(h*.115),right,(int)(h*.15)};text(dc,L"Forza Motorsport 5",r,(int)(h*.015),FW_SEMIBOLD,accent,DT_LEFT);
    r=(RECT){x,(int)(h*.151),right,(int)(h*.213)};text(dc,L"Settings",r,(int)(h*.043),FW_LIGHT,RGB(255,255,255),DT_LEFT);
    r=(RECT){x,top,right,top+step};u->rows[u->model->count]=r;
    text(dc,L"Start game",r,fs,FW_SEMIBOLD,accent,DT_LEFT);
    line(dc,x,r.bottom,right,u->model->focused==u->model->count?accent:RGB(58,60,65));
    for(i=0;i<u->model->count;i++) {
        FM5UIRow *row=&u->model->rows[i];BOOL selected=u->model->focused==i;
        COLORREF color=row->enabled?RGB(242,242,244):RGB(145,147,151);
        u->rows[i]=(RECT){x,top+((int)i+1)*step,right,top+((int)i+2)*step};r=u->rows[i];
        if(selected){r.left-=16;line(dc,r.left,r.top+step/2,r.left+5,accent);}
        r=u->rows[i];r.right=x+(right-x)*48/100;text(dc,row->label,r,fs,FW_NORMAL,color,DT_LEFT);
        r=u->rows[i];r.left=x+(right-x)*48/100;
        text(dc,row->choices[row->selected].label,r,fs,selected?FW_SEMIBOLD:FW_NORMAL,selected?accent:color,DT_RIGHT);
        line(dc,x,u->rows[i].bottom,right,RGB(58,60,65));
    }
    r=(RECT){x,(int)(h*.86),right,(int)(h*.905)};
    if(u->model->error[0])text(dc,u->model->error,r,(int)(h*.015),FW_NORMAL,accent,DT_LEFT);
    r=(RECT){x,top+((int)u->model->count+1)*step,right,top+((int)u->model->count+2)*step};
    u->rows[u->model->count+1]=r;
    text(dc,L"Quit",r,fs,FW_NORMAL,u->model->focused==u->model->count+1?accent:RGB(245,245,247),DT_LEFT);
    line(dc,x,r.bottom,right,u->model->focused==u->model->count+1?accent:RGB(58,60,65));
    r=(RECT){x,(int)(h*.94),right,(int)(h*.972)};
    text(dc,L"\x2191\x2193 Select   \x2190\x2192 Change   A / Enter Confirm",r,(int)(h*.014),FW_NORMAL,RGB(167,169,173),DT_LEFT);
}
static void activate(UI *u) {
    HRESULT result;
    if(u->model->focused<u->model->count){FM5NativeUIAction(u->model,4);return;}
    if(u->model->focused==u->model->count+1){u->finished=TRUE;return;}
    if(!u->model->commit){wcscpy_s(u->model->error,192,L"Settings integration is unavailable.");return;}
    result=u->model->commit(u->model,u->model->context);
    if(result==E_PENDING){u->pending_start=TRUE;wcscpy_s(u->model->error,192,L"Preparing graphics...");return;}
    u->pending_start=FALSE;
    if(result==S_OK){u->accepted=TRUE;u->finished=TRUE;}
    else if(!u->model->error[0])wcscpy_s(u->model->error,192,L"Could not save settings. Please try again.");
}
static void poll(UI *u) {
    DWORD i;ULONGLONG now=GetTickCount64();
    if(GetForegroundWindow()!=u->hwnd){ZeroMemory(u->buttons,sizeof(u->buttons));return;}
    for(i=0;i<4;i++){
        XINPUT_STATE state={0};WORD bits,edge;unsigned action=0;
        if(XInputGetState(i,&state)!=ERROR_SUCCESS){u->buttons[i]=0;continue;}
        bits=state.Gamepad.wButtons;
        if(state.Gamepad.sThumbLY>18000)bits|=XINPUT_GAMEPAD_DPAD_UP;
        if(state.Gamepad.sThumbLY< -18000)bits|=XINPUT_GAMEPAD_DPAD_DOWN;
        if(state.Gamepad.sThumbLX>18000)bits|=XINPUT_GAMEPAD_DPAD_RIGHT;
        if(state.Gamepad.sThumbLX< -18000)bits|=XINPUT_GAMEPAD_DPAD_LEFT;
        edge=(WORD)(bits&~u->buttons[i]);
        if(u->pending_start){
            if(edge&XINPUT_GAMEPAD_B){u->pending_start=FALSE;u->model->error[0]=0;}
            u->buttons[i]=bits;continue;
        }
        if(edge&(XINPUT_GAMEPAD_A|XINPUT_GAMEPAD_START))activate(u);
        else if(edge&XINPUT_GAMEPAD_B)u->model->focused=u->model->count+1;
        if(edge&15)u->repeats[i]=now+350;
        else if((bits&15)&&now>=u->repeats[i]){edge|=bits&15;u->repeats[i]=now+130;}
        if(edge&XINPUT_GAMEPAD_DPAD_UP)action=1;else if(edge&XINPUT_GAMEPAD_DPAD_DOWN)action=2;
        else if(edge&XINPUT_GAMEPAD_DPAD_LEFT)action=3;else if(edge&XINPUT_GAMEPAD_DPAD_RIGHT)action=4;
        if(action)FM5NativeUIAction(u->model,action);u->buttons[i]=bits;
    }
}
static LRESULT CALLBACK proc(HWND hwnd,UINT msg,WPARAM wp,LPARAM lp) {
    UI *u=(UI*)GetPropW(hwnd,property);unsigned a=0,i;
    if(!u)return DefWindowProcW(hwnd,msg,wp,lp);
    switch(msg){
    case WM_ERASEBKGND:return 1;
    case WM_PAINT:{PAINTSTRUCT ps;RECT r;HDC dc=BeginPaint(hwnd,&ps),mem=CreateCompatibleDC(dc);HBITMAP bm;HGDIOBJ old;
        GetClientRect(hwnd,&r);bm=CreateCompatibleBitmap(dc,r.right?r.right:1,r.bottom?r.bottom:1);old=SelectObject(mem,bm);
        render(u,mem,r.right,r.bottom);BitBlt(dc,0,0,r.right,r.bottom,mem,0,0,SRCCOPY);SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);EndPaint(hwnd,&ps);return 0;}
    case WM_SIZE:InvalidateRect(hwnd,NULL,FALSE);break;
    case WM_TIMER:if(wp==u->timer){poll(u);if(u->pending_start)activate(u);InvalidateRect(hwnd,NULL,FALSE);return 0;}break;
    case WM_KEYDOWN:
        if(u->pending_start){if(wp==VK_ESCAPE){u->pending_start=FALSE;u->model->error[0]=0;}InvalidateRect(hwnd,NULL,FALSE);return 0;}
        if(wp==VK_UP)a=1;else if(wp==VK_DOWN)a=2;else if(wp==VK_LEFT)a=3;else if(wp==VK_RIGHT)a=4;
        else if(wp==VK_RETURN||wp==VK_SPACE)activate(u);else if(wp==VK_ESCAPE)u->model->focused=u->model->count+1;
        if(a)FM5NativeUIAction(u->model,a);InvalidateRect(hwnd,NULL,FALSE);return 0;
    case WM_LBUTTONDOWN:{POINT p={(short)LOWORD(lp),(short)HIWORD(lp)};
        if(u->pending_start)return 0;
        for(i=0;i<u->model->count+2;i++)if(PtInRect(&u->rows[i],p)){u->model->focused=i;activate(u);InvalidateRect(hwnd,NULL,FALSE);break;}return 0;}
    case WM_CLOSE:u->finished=TRUE;return 0;
    }
    return CallWindowProcW(u->previous,hwnd,msg,wp,lp);
}
static HRESULT validate(FM5UIModel *m) {
    unsigned i;if(!m||!m->count||m->count>FM5_UI_MAX_ROWS||m->focused>m->count+1)return E_INVALIDARG;
    for(i=0;i<m->count;i++){FM5UIRow *r=&m->rows[i];if(!r->label||!r->count||r->count>FM5_UI_MAX_CHOICES||r->selected>=r->count)return E_INVALIDARG;}
    return S_OK;
}
HRESULT FM5RunNativePreGame(HWND window,FM5UIModel *m,const wchar_t *background) {
    UI u={0};HRESULT hr=validate(m),co;MSG msg;BOOL quit=FALSE;WPARAM quit_code=0;
    if(FAILED(hr)||!background||!IsWindow(window))return E_INVALIDARG;
    if(GetWindowThreadProcessId(window,NULL)!=GetCurrentThreadId()||GetPropW(window,property))return E_UNEXPECTED;
    co=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);if(FAILED(co)&&co!=RPC_E_CHANGED_MODE)return co;
    hr=image_load(background,&u.background,&u.bw,&u.bh);if(FAILED(hr))goto cleanup;
    u.hwnd=window;u.model=m;
    if(!SetPropW(window,property,&u)){hr=winerror();goto cleanup;}
    SetLastError(0);u.previous=(WNDPROC)SetWindowLongPtrW(window,GWLP_WNDPROC,(LONG_PTR)proc);
    if(!u.previous){hr=winerror();RemovePropW(window,property);goto cleanup;}
    /* With a non-NULL HWND, WM_TIMER carries the requested ID, not a generated
       return value. A zero requested ID silently lost polling on real Win32. */
    u.timer=(UINT_PTR)property;
    if(!SetTimer(window,u.timer,16,NULL)){u.timer=0;hr=winerror();goto restore;}
#ifndef FM5_NATIVE_UI_TEST
    ShowWindow(window,SW_SHOW);SetForegroundWindow(window);
#endif
    InvalidateRect(window,NULL,FALSE);
    while(!u.finished){int result=GetMessageW(&msg,NULL,0,0);if(result<=0){quit=result==0;quit_code=msg.wParam;hr=result<0?winerror():S_FALSE;break;}TranslateMessage(&msg);DispatchMessageW(&msg);}
    if(u.finished)hr=u.accepted?S_OK:S_FALSE;
restore:
    if(u.timer)KillTimer(window,u.timer);
    SetWindowLongPtrW(window,GWLP_WNDPROC,(LONG_PTR)u.previous);RemovePropW(window,property);
    if(quit)PostQuitMessage((int)quit_code);
cleanup:
    if(u.background)DeleteObject(u.background);if(SUCCEEDED(co))CoUninitialize();return hr;
}
HRESULT FM5RenderNativeUIPreview(FM5UIModel *m,const wchar_t *background,const wchar_t *output,unsigned w,unsigned h) {
    UI u={0};HRESULT hr=validate(m),co;HDC dc,mem;BITMAPINFO bi={0};HBITMAP bm;HGDIOBJ old;void *bits;HANDLE file;DWORD written;BITMAPFILEHEADER fh={0};
    if(FAILED(hr)||!w||!h||w>7680||h>4320)return E_INVALIDARG;
    co=CoInitializeEx(NULL,COINIT_APARTMENTTHREADED);if(FAILED(co)&&co!=RPC_E_CHANGED_MODE)return co;
    hr=image_load(background,&u.background,&u.bw,&u.bh);if(FAILED(hr))goto done;
    u.model=m;bi.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);bi.bmiHeader.biWidth=(LONG)w;bi.bmiHeader.biHeight=-(LONG)h;bi.bmiHeader.biPlanes=1;bi.bmiHeader.biBitCount=32;
    dc=GetDC(NULL);mem=CreateCompatibleDC(dc);bm=CreateDIBSection(dc,&bi,DIB_RGB_COLORS,&bits,NULL,0);ReleaseDC(NULL,dc);
    if(!bm){DeleteDC(mem);hr=E_OUTOFMEMORY;goto done;}old=SelectObject(mem,bm);render(&u,mem,(int)w,(int)h);
    file=CreateFileW(output,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
    if(file==INVALID_HANDLE_VALUE)hr=winerror();else{
        fh.bfType=0x4d42;fh.bfOffBits=sizeof(fh)+sizeof(BITMAPINFOHEADER);fh.bfSize=fh.bfOffBits+w*h*4;
        if(!WriteFile(file,&fh,sizeof(fh),&written,NULL)||written!=sizeof(fh)||!WriteFile(file,&bi.bmiHeader,sizeof(BITMAPINFOHEADER),&written,NULL)||written!=sizeof(BITMAPINFOHEADER)||!WriteFile(file,bits,w*h*4,&written,NULL)||written!=w*h*4)hr=E_FAIL;
        CloseHandle(file);
    }SelectObject(mem,old);DeleteObject(bm);DeleteDC(mem);
done:if(u.background)DeleteObject(u.background);if(SUCCEEDED(co))CoUninitialize();return hr;
}
