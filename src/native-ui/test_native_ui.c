#include "fm5_native_ui.h"
#include <stdio.h>
static unsigned checks;
static unsigned commits,pending_budget;static HRESULT commit_result;static HWND test_window;static unsigned phase;
static BOOL require_message;static volatile LONG message_seen;
static HRESULT do_commit(FM5UIModel *m,void *context){(void)m;(void)context;commits++;if(require_message&&!message_seen)return E_PENDING;if(pending_budget){pending_budget--;return E_PENDING;}return commit_result;}
static LRESULT CALLBACK original(HWND h,UINT m,WPARAM w,LPARAM l){if(m==WM_APP+71){InterlockedExchange(&message_seen,1);return 19;}return DefWindowProcW(h,m,w,l);}
static DWORD WINAPI send_worker(void *unused){(void)unused;Sleep(80);return (DWORD)SendMessageW(test_window,WM_APP+71,0,0);}
static VOID CALLBACK drive(HWND hwnd,UINT msg,UINT_PTR id,DWORD time){
    (void)hwnd;(void)msg;(void)time;
    if(phase==0){PostMessageW(test_window,WM_KEYDOWN,VK_RETURN,0);phase++;}
    else {KillTimer(NULL,id);if(FAILED(commit_result))PostMessageW(test_window,WM_CLOSE,0,0);}
}
#define CHECK(v) do{checks++;if(!(v)){fprintf(stderr,"FAIL %u: %s\n",checks,#v);return 1;}}while(0)
static void row(FM5UIModel *m,const wchar_t *label,const wchar_t *a,const wchar_t *b,unsigned selected) {
    FM5UIRow *r=&m->rows[m->count++];r->label=label;r->count=2;r->enabled=TRUE;
    r->choices[0]=(FM5UIChoice){a,0};r->choices[1]=(FM5UIChoice){b,1};r->selected=selected;
}
int wmain(int argc,wchar_t **argv) {
    FM5UIModel m={0};
    row(&m,L"Resolution",L"1920 \x00d7 1080",L"3840 \x00d7 2160",1);
    row(&m,L"Frame rate",L"60 FPS",L"120 FPS",0);
    row(&m,L"Graphics",L"Original",L"Ultra",1);
    row(&m,L"MSAA",L"2\x00d7",L"4\x00d7",1);
    row(&m,L"VSync",L"Off",L"On",1);
    row(&m,L"Display",L"Windowed",L"Fullscreen",1);
    row(&m,L"Language",L"Italian",L"English (US)",0);
    row(&m,L"Audio output",L"Stereo",L"Multichannel",1);
    CHECK(FM5NativeUIAction(&m,3));CHECK(m.rows[0].selected==0);
    CHECK(FM5NativeUIAction(&m,3));CHECK(m.rows[0].selected==1);
    CHECK(FM5NativeUIAction(&m,1));CHECK(m.focused==8);
    CHECK(FM5NativeUIAction(&m,2));CHECK(m.focused==0);
    m.focused=m.count;CHECK(FM5NativeUIAction(&m,1));CHECK(m.focused==m.count+1);
    CHECK(FM5NativeUIAction(&m,2));CHECK(m.focused==m.count);
    CHECK(FM5NativeUIAction(&m,2));CHECK(m.focused==0);
    m.focused=m.count-1;CHECK(FM5NativeUIAction(&m,2));CHECK(m.focused==m.count+1);
    m.focused=6;CHECK(FM5NativeUIAction(&m,4));CHECK(m.rows[6].selected==1);
    m.focused=m.count;CHECK(!FM5NativeUIAction(&m,4));m.focused=0;
    CHECK(!FM5NativeUIAction(NULL,1));CHECK(FM5RunNativePreGame(NULL,&m,L"missing")==E_INVALIDARG);
    if(argc==3){WNDCLASSW wc={0};HRESULT hr;UINT_PTR driver;WNDPROC before;
        CHECK(SUCCEEDED(FM5RenderNativeUIPreview(&m,argv[1],argv[2],1920,1080)));
        wc.lpfnWndProc=original;wc.hInstance=GetModuleHandleW(NULL);wc.lpszClassName=L"FM5OwnedUIHarness";CHECK(RegisterClassW(&wc)!=0);
        test_window=CreateWindowW(wc.lpszClassName,L"Native UI harness",WS_OVERLAPPEDWINDOW,0,0,640,480,NULL,NULL,wc.hInstance,NULL);CHECK(test_window!=NULL);
        before=(WNDPROC)GetWindowLongPtrW(test_window,GWLP_WNDPROC);m.commit=do_commit;m.focused=m.count;
        commit_result=S_OK;phase=0;driver=SetTimer(NULL,0,20,drive);CHECK(driver!=0);
        hr=FM5RunNativePreGame(test_window,&m,argv[1]);KillTimer(NULL,driver);
        CHECK(hr==S_OK);CHECK(commits==1);CHECK(IsWindow(test_window));CHECK((WNDPROC)GetWindowLongPtrW(test_window,GWLP_WNDPROC)==before);
        CHECK(GetPropW(test_window,L"ForzaMastro5.NativePreGame.State.v1")==NULL);
        commit_result=E_FAIL;phase=0;driver=SetTimer(NULL,0,20,drive);m.focused=m.count;CHECK(driver!=0);
        hr=FM5RunNativePreGame(test_window,&m,argv[1]);KillTimer(NULL,driver);
        CHECK(hr==S_FALSE);CHECK(commits==2);CHECK(m.error[0]!=0);CHECK(IsWindow(test_window));CHECK((WNDPROC)GetWindowLongPtrW(test_window,GWLP_WNDPROC)==before);
        CHECK(FM5RunNativePreGame(test_window,&m,L"Z:\\missing-fm5-ui.png")<0);CHECK((WNDPROC)GetWindowLongPtrW(test_window,GWLP_WNDPROC)==before);
        commit_result=S_OK;pending_budget=3;phase=0;driver=SetTimer(NULL,0,20,drive);m.focused=m.count;CHECK(driver!=0);
        hr=FM5RunNativePreGame(test_window,&m,argv[1]);KillTimer(NULL,driver);
        CHECK(hr==S_OK);CHECK(pending_budget==0);CHECK(commits==6);CHECK(IsWindow(test_window));
        CHECK((WNDPROC)GetWindowLongPtrW(test_window,GWLP_WNDPROC)==before);
        {HANDLE worker;DWORD code=0;require_message=TRUE;message_seen=0;phase=0;m.focused=m.count;
        driver=SetTimer(NULL,0,20,drive);CHECK(driver!=0);worker=CreateThread(NULL,0,send_worker,NULL,0,NULL);CHECK(worker!=NULL);
        hr=FM5RunNativePreGame(test_window,&m,argv[1]);KillTimer(NULL,driver);CHECK(hr==S_OK);CHECK(message_seen==1);
        CHECK(WaitForSingleObject(worker,1000)==WAIT_OBJECT_0);CHECK(GetExitCodeThread(worker,&code));CHECK(code==19);CloseHandle(worker);require_message=FALSE;
        CHECK(IsWindow(test_window));CHECK((WNDPROC)GetWindowLongPtrW(test_window,GWLP_WNDPROC)==before);}
        DestroyWindow(test_window);UnregisterClassW(wc.lpszClassName,wc.hInstance);
    }
    printf("{\"checks\":%u,\"ok\":true}\n",checks);return 0;
}
