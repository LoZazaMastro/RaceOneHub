/* The production binding calls these exact native ABIs. This fixture replaces
   their code entrypoints with owned test thunks; it never opens Azimuth/game. */
#include "fm5_native_binding.c"
static unsigned checks,clones,moves,destroys,saves;static unsigned fields[20];
#define CHECK(v) do{checks++;if(!(v)){fprintf(stderr,"FAIL %u: %s\n",checks,#v);return 1;}}while(0)
static void *mock_clone(void *to,const void *from){clones++;return memcpy(to,from,0x100);}
static void mock_destroy(void *p){(void)p;destroys++;}
static void *mock_move(void *to,void *from){moves++;return memcpy(to,from,0x100);}
static void *mock_assign(void *p,const wchar_t *s,size_t n){NativeString *v=p;v->size=n;v->capacity=7;memcpy(v->data.small,s,(n+1)*sizeof(wchar_t));return p;}
static void mock_save(void *path,void *settings,unsigned field){(void)path;(void)settings;fields[saves++]=field;}
static void *mock_preset(void *out,unsigned level){unsigned *v=out;unsigned i;for(i=0;i<6;i++)v[i]=level;return out;}
static BOOL snapshot_ready;static void *mock_snapshot(void *out){BYTE *p=out;memset(p,0,24);p[0]=(BYTE)snapshot_ready;*(uint64_t*)(p+8)=3;*(uint64_t*)(p+16)=9;return out;}
static unsigned parsed_count,game_moves,game_destroys;static uintptr_t expected_profile;
static void *mock_game_parse(void *out,void *path,const uintptr_t *profiles){
    if(wcscmp(str(path),L"C:\\FM5")||profiles[0]!=expected_profile||profiles[1]!=2)RaiseException(0xe0000001,0,0,NULL);
    memset(out,0x5c,0x1a0);parsed_count++;return out;
}
static void *mock_game_move(void *out,void *from){game_moves++;return memcpy(out,from,0x1a0);}
static void mock_game_destroy(void *out){(void)out;game_destroys++;}
static void thunk(BYTE *host,unsigned rva,uintptr_t fn){BYTE j[12]={0x48,0xb8,0,0,0,0,0,0,0,0,0xff,0xe0};memcpy(j+2,&fn,8);memcpy(host+rva,j,sizeof(j));}
int main(void){Binding b={0};BYTE settings[0x100],before[0x100];DWORD old;unsigned i;HRESULT hr;struct{uint64_t pre;BYTE data[0x1c0];uint64_t post;} game;NativeString resolved={0};
    b.host=VirtualAlloc(NULL,0x90000,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);CHECK(b.host!=NULL);
    thunk(b.host,0x13ea0,(uintptr_t)mock_clone);thunk(b.host,0x6eb0,(uintptr_t)mock_destroy);thunk(b.host,0x6f00,(uintptr_t)mock_move);
    thunk(b.host,0x7bd0,(uintptr_t)mock_assign);thunk(b.host,0x7f6f0,(uintptr_t)mock_save);thunk(b.host,0x31e0,(uintptr_t)mock_preset);
    thunk(b.host,0x875b0,(uintptr_t)mock_game_parse);thunk(b.host,0x7140,(uintptr_t)mock_game_move);thunk(b.host,0x7070,(uintptr_t)mock_game_destroy);
    thunk(b.host,0x57f80,(uintptr_t)mock_snapshot);
    CHECK(VirtualProtect(b.host,0x90000,PAGE_EXECUTE_READ,&old));CHECK(FlushInstructionCache(GetCurrentProcess(),b.host,0x90000));
    memset(settings,0xa5,sizeof(settings));memcpy(before,settings,sizeof(settings));b.settings=settings;b.path=(void*)0x1234;
    b.model.count=8;b.initial_graphics=1;
    for(i=0;i<8;i++){b.model.rows[i].count=1;b.model.rows[i].choices[0].value=0;}
    b.model.rows[0].choices[0].value=2160;b.model.rows[1].choices[0].value=120;
    b.model.rows[2].choices[0].value=3;b.model.rows[3].choices[0].value=4;
    b.model.rows[4].choices[0].value=1;b.model.rows[5].choices[0].value=1;
    b.model.rows[6].choices[0].value=1;b.model.rows[7].choices[0].value=2;
    b.model.preparation=preparation;snapshot_ready=FALSE;hr=commit(&b.model,&b);
    CHECK(hr==E_PENDING);CHECK(clones==0);CHECK(saves==0);CHECK(!memcmp(settings,before,sizeof(settings)));
    {uint64_t done,total;CHECK(!preparation(&b,&done,&total));CHECK(done==3);CHECK(total==9);}
    snapshot_ready=TRUE;
    hr=commit(&b.model,&b);CHECK(hr==S_OK);CHECK(clones==1);CHECK(moves==1);CHECK(destroys==1);
    CHECK(*(unsigned*)(settings)==2160);CHECK(*(unsigned*)(settings+8)==120);CHECK(*(unsigned*)(settings+4)==4);
    CHECK(settings[0xc]==1);CHECK(settings[0xd]==1);CHECK(settings[0xe]==1);
    CHECK(*(unsigned*)(settings+0xe8)==2);CHECK(!wcscmp(str((NativeString*)(settings+0x18)),L"en-US"));
    CHECK(!wcscmp(str((NativeString*)(settings+0x38)),L"system"));CHECK(!memcmp(settings+0x58,before+0x58,0x78));CHECK(!memcmp(settings+0xec,before+0xec,0x14));
    CHECK(saves==10);for(i=0;i<9;i++)CHECK(fields[i]==i+1);CHECK(fields[9]==0x10);
    for(i=0;i<6;i++)CHECK(*(unsigned*)(settings+0xd0+i*4)==3);
    game.pre=0x12345678;game.post=0x87654321;memset(game.data,0,sizeof(game.data));mock_assign(&resolved,L"C:\\FM5",6);expected_profile=(uintptr_t)(b.host+0x29a1c0);
    populate_game_folder(&b,game.data,&resolved);CHECK(parsed_count==1);CHECK(game_moves==1);CHECK(game_destroys==1);
    CHECK(!wcscmp(str((NativeString*)game.data),L"C:\\FM5"));CHECK(game.pre==0x12345678);CHECK(game.post==0x87654321);
    for(i=0;i<0x1a0;i++)if(game.data[0x20+i]!=0x5c)break;CHECK(i==0x1a0);
    {static const unsigned heights[]={1080,1440,2160,2880,3240,3780,4320};
     static const wchar_t *expected_labels[]={L"1080p (1920x1080)",L"1440p (2560x1440)",L"4K (3840x2160)",L"5K (5120x2880)",L"6K (5760x3240)",L"7K (6720x3780)",L"8K (7680x4320)"};
     unsigned j;
     for(j=0;j<7;j++){Binding choices={0};add_resolutions(&choices,heights[j]);
       CHECK(choices.model.count==1);CHECK(choices.model.rows[0].count==7);CHECK(choices.model.rows[0].selected==j);
       for(i=0;i<7;i++){CHECK(choices.model.rows[0].choices[i].value==heights[i]);CHECK(!wcscmp(choices.model.rows[0].choices[i].label,expected_labels[i]));}
       b.model.rows[0]=choices.model.rows[0];saves=0;
       CHECK(commit(&b.model,&b)==S_OK);CHECK(*(unsigned*)settings==heights[j]);CHECK(saves==10);CHECK(fields[0]==1);
     }}
    {static const unsigned requested[]={1,2,4,8},expected[]={1,2,4,4};unsigned j;
     for(j=0;j<4;j++){Binding choices={0};add_antialiasing(&choices,requested[j]);
       CHECK(choices.model.count==1);CHECK(choices.model.rows[0].count==3);
       CHECK(value(&choices.model,0)==expected[j]);
       for(i=0;i<3;i++)CHECK(choices.model.rows[0].choices[i].value!=8);
       b.model.rows[3]=choices.model.rows[0];saves=0;
       CHECK(commit(&b.model,&b)==S_OK);CHECK(*(unsigned*)(settings+4)==expected[j]);CHECK(saves==10);
     }
     b.model.rows[3].choices[b.model.rows[3].selected].value=8;saves=0;
     CHECK(commit(&b.model,&b)==E_INVALIDARG);CHECK(saves==0);CHECK(*(unsigned*)(settings+4)==4);
    }
    VirtualFree(b.host,0,MEM_RELEASE);printf("{\"checks\":%u,\"ok\":true}\n",checks);return 0;
}
