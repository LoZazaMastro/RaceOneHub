#define WIN32_LEAN_AND_MEAN
#include "fm5_color_observer.h"
#pragma warning(push)
#pragma warning(disable:4201)
#include <d3d12.h>
#pragma warning(pop)
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "color_guard_fixture.h"

static BYTE image[0x340000],ui[0x700],resource_object[0x200];
static BYTE view[0x200],backing[0x100],device[0x20000],shader_object[0x100],program[0x200];
static BYTE *context=device+0x6f40;
static void *context_table[128],*resource_table[16],*view_table[16];
static char last_line[65536];
static char last_before[65536];
static UINT writes,locks,checks;
static LONGLONG ticks;
struct NativeMutex { SRWLOCK lock; UINT owner,depth; };
static NativeMutex mutex={SRWLOCK_INIT,0,0};
#define CHECK(x) do {++checks;if(!(x)){fprintf(stderr,"failed line %d: %s\n",__LINE__,#x);ExitProcess(1);}}while(0)
static void setp(void *base,SIZE_T at,const void *value){memcpy((BYTE*)base+at,&value,8);}
static void setu(void *base,SIZE_T at,UINT value){memcpy((BYTE*)base+at,&value,4);}
static void *unwrap(void *value){CHECK(value==(void*)0x1234);return context;}
static LONGLONG now(){return ticks;}
static void write_line(const char *line,DWORD length){
 CHECK(mutex.depth==0 && mutex.owner==0);
 CHECK(length<sizeof(last_line));memcpy(last_line,line,length);last_line[length]=0;
 if(strstr(last_line,"\"phase\":\"before_native_draw\""))memcpy(last_before,last_line,length+1);
 else ++writes;SetLastError(999);
}
static FM5ColorLock *acquire(FM5ColorLock *record,void *value){
 CHECK(value==context);AcquireSRWLockExclusive(&mutex.lock);
 mutex.depth=1;mutex.owner=GetCurrentThreadId();record->held=1;record->mutex=&mutex;++locks;return record;
}
extern "C" void *FM5UnwrapPerformanceContext(void *value){return unwrap(value);}
extern "C" void FM5DiagnosticWriteJsonLine(const char *value,DWORD length){write_line(value,length);}
static void configure(BOOL enabled=TRUE){
 FM5ColorObserverTestConfigure(image,sizeof(image),ui,enabled,unwrap,now,write_line,acquire);
 writes=locks=0;ticks=1000;last_line[0]=0;
}
static void observe_postprocess(void *value){FM5ObservePostprocessDraw(value,FALSE);FM5ObservePostprocessDraw(value,TRUE);}
static DWORD WINAPI other_thread_unmap(void *resource){FM5ObservePostprocessUnmap(context,resource,0);return 0;}
static UINT preparation_calls;static void *preparation_result=(void*)0x1234567887654321ull;
static BOOL preparation_throws;
static void *WINAPI prepare_native(void *value,BYTE flag,void *opaque){
 CHECK(value==context && flag==0xab && opaque==(void*)0x8877665544332211ull);
 ++preparation_calls;if(preparation_throws)RaiseException(0xe0f50123,0,0,NULL);
 setu(context,0xb5d4,123);SetLastError(812);return preparation_result;
}
int main(){
 for(UINT i=0;i<_countof(fixture_guards);++i)
  memcpy(image+fixture_guards[i].rva,fixture_guards[i].bytes,16);
 CHECK(SUCCEEDED(FM5ColorObserverTestGuard(image,sizeof(image))));
 CHECK(FAILED(FM5ColorObserverTestGuard(image,0x1000)));
 CHECK(FM5ColorObserverTestGuard(NULL,sizeof(image))==E_POINTER);
 for(UINT i=0;i<_countof(fixture_guards);++i)for(UINT b=0;b<16;++b){
  image[fixture_guards[i].rva+b]^=1;
  CHECK(FAILED(FM5ColorObserverTestGuard(image,sizeof(image))));
  image[fixture_guards[i].rva+b]^=1;
 }
 setp(ui,0x18,(void*)0x1234);setp(context,0,context_table);
 context_table[3]=image+0xdbe90;context[0xc48]=1;setp(context,0xc40,device);
 setu(context,0x6fa0,0xf0ff);
 setp(resource_object,0,resource_table);resource_table[12]=image+0xeab90;
 setp(resource_object,0x30,device);setu(resource_object,0x38,12);
 setu(resource_object,0x70,16);setu(resource_object,0x74,16);setu(resource_object,0x80,28);
 setp(resource_object,0x40,backing);
 setp(view,0,view_table);view_table[8]=image+0x141910;
 setu(view,0x38,11);
 setp(view,0x30,device);setp(view,0x48,resource_object);setu(view,0x108,28);
 setp(context,0x4100+1*8,view);setp(context,0x4100+(4*128)*8,view);
 setp(context,0x4100+(4*128+127)*8,view);
 setp(shader_object,0x30,device);setu(shader_object,0x38,8);setu(shader_object,0x50,4);
 setp(shader_object,0x58,program);memset(program+0x168,0xcd,24);
 memset(program+0x180,0xab,32);setp(context,0x40f0,shader_object);
#ifndef FM5_COLOR_CPU_ONLY
 setp(context,0x8a48+(4*14+3)*16,resource_object);
 ID3D12Device *gpu=NULL;ID3D12Resource *physical=NULL;
 CHECK(SUCCEEDED(D3D12CreateDevice(NULL,D3D_FEATURE_LEVEL_11_0,IID_PPV_ARGS(&gpu))));
 D3D12_HEAP_PROPERTIES heap={};heap.Type=D3D12_HEAP_TYPE_DEFAULT;
 D3D12_RESOURCE_DESC description={};description.Dimension=D3D12_RESOURCE_DIMENSION_TEXTURE2D;
 description.Width=16;description.Height=16;description.DepthOrArraySize=1;description.MipLevels=1;
 description.Format=DXGI_FORMAT_R8G8B8A8_UNORM;description.SampleDesc.Count=1;
 CHECK(SUCCEEDED(gpu->CreateCommittedResource(&heap,D3D12_HEAP_FLAG_NONE,&description,
   D3D12_RESOURCE_STATE_COMMON,NULL,IID_PPV_ARGS(&physical))));
 setp(backing,0x10,physical);
#endif
 configure(FALSE);FM5ObserveColorBeforeUi(ui);CHECK(writes==0 && locks==0);
 configure();FM5ObserveColorBeforeUi(NULL);FM5ObserveColorBeforeUi(ui+1);CHECK(writes==0);
 ui[0x5f0]=1;FM5ObserveColorBeforeUi(ui);CHECK(writes==0);ui[0x5f0]=0;
 BYTE saved[0xd800];memcpy(saved,context,0xd800);
 SetLastError(123);FM5ObserveColorBeforeUi(ui);CHECK(GetLastError()==123);
 CHECK(writes==1 && locks==1);CHECK(!memcmp(saved,context,0xd800));
 CHECK(strstr(last_line,"\"event\":\"fm5_color_before_ui\""));
 CHECK(strstr(last_line,"\"stage\":4"));CHECK(strstr(last_line,"\"slot\":127"));
 CHECK(strstr(last_line,"\"sample_mask\":61695"));
 CHECK(strstr(last_line,"\"binding_signatures_at180\":\"abababab"));
 CHECK(strstr(last_line,"\"native_bytecode_key_at168\":\"cdcdcdcd"));
#ifndef FM5_COLOR_CPU_ONLY
 CHECK(strstr(last_line,"\"actual_description\":{\"dimension\":3,\"width\":16,\"height\":16"));
 CHECK(strstr(last_line,"\"format\":28,\"samples\":1,\"quality\":0"));
#endif
 FILE *output=NULL;CHECK(fopen_s(&output,"color-observer-sample.json","wb")==0);
 fwrite(last_line,1,strlen(last_line),output);fclose(output);
 ticks=1999;FM5ObserveColorBeforeUi(ui);CHECK(writes==1);
 ticks=2000;FM5ObserveColorBeforeUi(ui);CHECK(writes==2 && locks==2);
 configure();setu(context,0xd2c0,1);setp(context,0x8a48,(void*)1);FM5ObserveColorBeforeUi(ui);
 CHECK(writes==1 && strstr(last_line,"\"logical_cbv_cache_active\":false") && !strstr(last_line,"unavailable"));
 setu(context,0xd2c0,0);setp(context,0x8a48,NULL);
 configure();context[0xc49]=1;FM5ObserveColorBeforeUi(ui);
 CHECK(writes==1 && locks==0 && strstr(last_line,"unsupported_cache_mode"));context[0xc49]=0;
 configure();context_table[3]=NULL;FM5ObserveColorBeforeUi(ui);
 CHECK(writes==1 && locks==0 && strstr(last_line,"unverified_context"));context_table[3]=image+0xdbe90;
 configure();setp(context,0x4100,(void*)1);FM5ObserveColorBeforeUi(ui);
 CHECK(writes==1 && locks==1 && mutex.depth==0 && strstr(last_line,"unavailable"));setp(context,0x4100,NULL);
 configure();image[fixture_guards[0].rva]^=1;configure();FM5ObserveColorBeforeUi(ui);
 CHECK(writes==0 && locks==0);image[fixture_guards[0].rva]^=1;
 configure();for(UINT n=0;n<1202;++n){ticks=(LONGLONG)n*1000;FM5ObserveColorBeforeUi(ui);}
 CHECK(writes==1200 && locks==1200);
 /* Exact original shader input classification, including stripped containers. */
 BYTE original[80000];SIZE_T original_size=0;
 const UINT family_ids[10]={2,2,2,2,1,1,3,3,3,3};
 CHECK(FM5ColorObserverTestClassify(NULL,0)==0);
 for(UINT n=0;n<10;++n){char path[64];sprintf_s(path,"original-family-%u.bin",n);
  FILE *input=NULL;CHECK(fopen_s(&input,path,"rb")==0);
  SIZE_T size=fread(original,1,sizeof(original),input);fclose(input);
  CHECK(size>32 && FM5ColorObserverTestClassify(original,size)==family_ids[n]);
  original[size-1]^=1;CHECK(FM5ColorObserverTestClassify(original,size)==0);original[size-1]^=1;
  CHECK(FM5ColorObserverTestClassify(original,size-1)==0);original_size=size;
 }
 configure();observe_postprocess(context);CHECK(writes==0);
 FM5ObservePixelShaderCreation(shader_object,original,original_size);
 CHECK(writes==1 && strstr(last_line,"fm5_postprocess_shader_tag"));writes=0;
 observe_postprocess(context);CHECK(writes==1 && strstr(last_line,"fm5_postprocess_draw"));
 CHECK(strstr(last_line,"\"family\":3") && strstr(last_line,"no_completed_observed_upload")==NULL);
 ticks+=100;observe_postprocess(context);CHECK(writes==1);
 ticks+=1000;FM5ObservePostprocessDraw(context,FALSE);
 FM5ObservePostprocessDraw(context,FALSE);FM5ObservePostprocessDraw(context,TRUE);
 CHECK(writes==1);FM5ObservePostprocessDraw(context,TRUE);CHECK(writes==2);
 FM5ObservePostprocessDraw(context,TRUE);CHECK(writes==2); /* Unpaired after cannot create an event. */
 /* Key padding is deliberately not shader identity. */
 program[0x168+23]^=1;ticks+=1000;observe_postprocess(context);CHECK(writes==3);
 program[0x168]^=1;ticks+=1000;observe_postprocess(context);CHECK(writes==3);program[0x168]^=1;
 BYTE cb[0x200]={},payload[1664],sampler[0x220]={};
 for(UINT n=0;n<sizeof(payload);++n)payload[n]=(BYTE)n;
 setu(cb,0x38,5);setu(cb,0x3c,98);setp(cb,0x30,device);
 setu(cb,0x70,1664);setu(cb,0x74,2);setu(cb,0x78,4);setu(cb,0x7c,0x10000);
 setp(context,0x8a48+(4*14+1)*16,cb);
 D3D12_RANGE unused={};(void)unused;
 struct {void *data;UINT row,depth;} mapped={payload,1664,1664};
 FM5ObservePostprocessMap(context,cb,0,4,E_FAIL,&mapped);
 FM5ObservePostprocessUnmap(context,cb,0);ticks+=1000;observe_postprocess(context);
 CHECK(strstr(last_line,"no_completed_observed_upload"));
 FM5ObservePostprocessMap(context,cb,0,4,S_OK,&mapped);
 ticks+=1000;observe_postprocess(context);CHECK(strstr(last_line,"no_completed_observed_upload"));
 SetLastError(432);FM5ObservePostprocessUnmap(context,cb,0);CHECK(GetLastError()==432);
 ticks+=1000;observe_postprocess(context);
 CHECK(strstr(last_line,"successful_WRITE_DISCARD_before_Unmap") && strstr(last_line,"00010203040506070809"));
 CHECK(strstr(last_line,"\"binding_rename_correspondence_verified\":false"));
 FILE *post_sample=NULL;CHECK(fopen_s(&post_sample,"postprocess-observer-sample.json","wb")==0);
 fwrite(last_line,1,strlen(last_line),post_sample);fclose(post_sample);
 CHECK(fopen_s(&post_sample,"postprocess-observer-before-sample.json","wb")==0);
 fwrite(last_before,1,strlen(last_before),post_sample);fclose(post_sample);
 payload[0]=0xfe;ticks+=1000;observe_postprocess(context);
 CHECK(strstr(last_line,"\"bytes\":\"000102")); /* copied before Unmap, not retained pointer. */
 setu(cb,0x3c,99);ticks+=1000;observe_postprocess(context);CHECK(strstr(last_line,"no_completed_observed_upload"));
 setu(cb,0x3c,98);FM5ObservePostprocessMap(context,cb,0,2,S_OK,&mapped);
 FM5ObservePostprocessUnmap(context,cb,0);ticks+=1000;observe_postprocess(context);CHECK(strstr(last_line,"no_completed_observed_upload"));
 FM5ObservePostprocessMap(context,cb,0,4,S_OK,&mapped);
 HANDLE thread=CreateThread(NULL,0,other_thread_unmap,cb,0,NULL);CHECK(thread!=NULL);
 CHECK(WaitForSingleObject(thread,5000)==WAIT_OBJECT_0);CloseHandle(thread);
 ticks+=1000;observe_postprocess(context);CHECK(strstr(last_line,"no_completed_observed_upload"));
 FM5ObservePostprocessUnmap(context,cb,0);ticks+=1000;observe_postprocess(context);
 CHECK(strstr(last_line,"\"bytes\":\"fe0102"));
 BYTE second_cb[sizeof(cb)];memcpy(second_cb,cb,sizeof(cb));setu(second_cb,0x3c,100);
 BYTE second_payload[1664];memset(second_payload,0x77,sizeof(second_payload));
 struct {void *data;UINT row,depth;} second_map={second_payload,1664,1664};
 FM5ObservePostprocessMap(context,cb,0,4,S_OK,&mapped);
 FM5ObservePostprocessMap(context,second_cb,0,4,S_OK,&second_map);
 FM5ObservePostprocessUnmap(context,second_cb,0);FM5ObservePostprocessUnmap(context,cb,0);
 ticks+=1000;observe_postprocess(context);CHECK(strstr(last_line,"\"bytes\":\"fe0102"));
 setp(context,0x8a48+(4*14+1)*16,second_cb);ticks+=1000;observe_postprocess(context);
 CHECK(strstr(last_line,"\"bytes\":\"777777"));
 /* Invalid diagnostic memory cannot escape, retain a pointer, or leak the cache lock. */
 second_map.data=(void*)1;FM5ObservePostprocessMap(context,second_cb,0,4,S_OK,&second_map);
 FM5ObservePostprocessUnmap(context,second_cb,0);ticks+=1000;observe_postprocess(context);
 CHECK(strstr(last_line,"no_completed_observed_upload"));
 BYTE map_owner[0x280]={},buckets[64]={},node[0x20]={},allocation[0x40]={};
 setp(map_owner,0x250,buckets);setp(map_owner,0x240,map_owner);setp(map_owner,0x268,(void*)3);
 setp(context,0xd330,map_owner);second_cb[0x88]=1;
 unsigned long long hash=14695981039346656037ull;
 UINT identity=100;
 for(UINT n=0;n<4;++n){hash^=(identity>>(n*8))&255;hash*=1099511628211ull;}
 SIZE_T bucket=(SIZE_T)(hash&3)*16;
 setp(buckets,bucket,node);setp(buckets,bucket+8,node);setu(node,0x10,100);setp(node,0x18,allocation);
 setp(allocation,0x20,second_payload);setp(allocation,0x28,(void*)(UINT_PTR)0xdeadbeef);setp(allocation,0x30,backing);
 ticks+=1000;observe_postprocess(context);
 CHECK(strstr(last_line,"read_only_context_d330_existing_key")&&strstr(last_line,"efbeadde00000000"));
 setu(node,0x10,999);ticks+=1000;observe_postprocess(context);CHECK(strstr(last_line,"\"renamed_allocation\":null"));
 setp(buckets,bucket,NULL);setp(node,8,node);ticks+=1000;observe_postprocess(context);
 CHECK(strstr(last_line,"\"renamed_allocation\":null")); /* bounded cycle, no native insertion. */
 setu(sampler,0x38,10);setp(sampler,0x30,device);memcpy(sampler+0x40,image+0x29f558,16);
 memset(sampler+0x1b4,0xaa,52);setp(context,0x6820,sampler);
 ticks+=1000;observe_postprocess(context);CHECK(strstr(last_line,"\"description_hex\":\"aaaaaaaa"));
 sampler[0x40]^=1;ticks+=1000;observe_postprocess(context);CHECK(strstr(last_line,"unverified_sampler"));
 configure(FALSE);FM5ObservePixelShaderCreation((void*)1,(void*)1,80000);
 FM5ObservePostprocessMap((void*)1,(void*)1,0,4,S_OK,(void*)1);FM5ObservePostprocessUnmap((void*)1,(void*)1,0);
 observe_postprocess((void*)1);CHECK(writes==0);
 /* Native preparation boundary is the sole supported immediate-context read. */
 configure();FM5ColorObserverTestPreparation(prepare_native);
 FM5ObservePixelShaderCreation(shader_object,original,original_size);writes=0;
 context[0xc48]=0;setu(context,0xb5d0,123);setu(context,0xb5d4,122);
 CHECK(FM5PrepareGraphicsObserved(context,0xab,(void*)0x8877665544332211ull)==preparation_result);
 CHECK(preparation_calls==1 && GetLastError()==812 && writes==1 && locks==0);
 CHECK(strstr(last_line,"after_native_preparation") && strstr(last_line,"\"vertex\":{\"stage\":0") &&
       strstr(last_line,"\"cbv_version_counters\":[123,123]") && strstr(last_line,"\"context_lock_held\":false"));
 BYTE view_backing[0x20]={},logical_backing[0x20]={},blend[0x160]={};
 setp(view,0x50,view_backing);setp(resource_object,0x40,logical_backing);
 /* Association metadata is read without invoking a COM method on the view's backing. */
 setp(view_backing,0x10,NULL);setp(logical_backing,0x10,NULL);
 setp(context,0x4120,view);setp(context,0x6f88,blend);
 setu(blend,0x38,10);setp(blend,0x30,device);memcpy(blend+0x40,image+0x29fba8,16);memset(blend+0x50,0x5a,264);
 setu(context,0x6fa0,0xabcdef01);memset(context+0x6f90,0x3c,16);
 ticks+=1000;CHECK(FM5PrepareGraphicsObserved(context,0xab,(void*)0x8877665544332211ull)==preparation_result);
 CHECK(strstr(last_line,"\"view_kind\":11") && strstr(last_line,"\"view_owner_matches\":true") &&
  strstr(last_line,"\"native_get_resource_verified\":true") && strstr(last_line,"\"cached_and_logical_physical_match\":null"));
 CHECK(strstr(last_line,"\"sample_mask\":2882400001")&&strstr(last_line,"\"blend_factor_hex\":\"3c3c3c3c"));
 CHECK(strstr(last_line,"\"description_verified\":true,\"description_hex\":\"5a5a5a5a"));
 char allocation_line[2048];setp(view_backing,0x10,(void*)0x1111111122222222ull);
 setp(logical_backing,0x10,(void*)0x1111111122222222ull);
 CHECK(FM5ColorObserverTestViewMetadata(view,device,allocation_line,sizeof(allocation_line)));
 CHECK(strstr(allocation_line,"\"cached_and_logical_physical_match\":true") && strstr(allocation_line,"\"allocation_layout_verified\":true"));
 setp(view_backing,0x10,(void*)0x3333333344444444ull);
 CHECK(FM5ColorObserverTestViewMetadata(view,device,allocation_line,sizeof(allocation_line)));
 CHECK(strstr(allocation_line,"\"cached_and_logical_physical_match\":false"));
 CHECK(fopen_s(&post_sample,"view-allocation-sample.json","wb")==0);
 fwrite(allocation_line,1,strlen(allocation_line),post_sample);fclose(post_sample);
 view_table[8]=NULL;setp(view,0x50,(void*)1);
 CHECK(FM5ColorObserverTestViewMetadata(view,device,allocation_line,sizeof(allocation_line)));
 CHECK(strstr(allocation_line,"\"native_get_resource_verified\":false") && !strstr(allocation_line,"view_cached_backing"));
 view_table[8]=image+0x141910;setp(view,0x50,view_backing);
 setp(view_backing,0x10,NULL);setp(logical_backing,0x10,NULL);
 blend[0x40]^=1;ticks+=1000;FM5PrepareGraphicsObserved(context,0xab,(void*)0x8877665544332211ull);
 CHECK(strstr(last_line,"\"description_verified\":false"));blend[0x40]^=1;
 setp(context,0x6f88,NULL);setp(view,0x50,NULL);setp(resource_object,0x40,backing);setp(context,0x4120,NULL);
 CHECK(fopen_s(&post_sample,"postprocess-prepared-sample.json","wb")==0);
 fwrite(last_line,1,strlen(last_line),post_sample);fclose(post_sample);
 preparation_result=NULL;ticks+=1000;
 CHECK(FM5PrepareGraphicsObserved(context,0xab,(void*)0x8877665544332211ull)==NULL && preparation_calls==4 && writes==3);
 preparation_result=(void*)0x1234567887654321ull;preparation_throws=TRUE;BOOL propagated=FALSE;
 __try {FM5PrepareGraphicsObserved(context,0xab,(void*)0x8877665544332211ull);}
 __except(GetExceptionCode()==0xe0f50123?EXCEPTION_EXECUTE_HANDLER:EXCEPTION_CONTINUE_SEARCH){propagated=TRUE;}
 CHECK(propagated && preparation_calls==5 && writes==3);preparation_throws=FALSE;
 configure(FALSE);CHECK(FM5PrepareGraphicsObserved(context,0xab,(void*)0x8877665544332211ull)==preparation_result);
 CHECK(preparation_calls==6 && writes==0 && GetLastError()==812);
 image[0x12a6d0]^=1;configure();
 CHECK(FM5PrepareGraphicsObserved(context,0xab,(void*)0x8877665544332211ull)==preparation_result && writes==0);
 image[0x12a6d0]^=1;context[0xc48]=1;
 BYTE actual_buffer[0x200]={},actual_record[0x40]={},actual_payload[1664];char bound_json[8000];
 memset(actual_payload,0x62,sizeof(actual_payload));
 const SIZE_T psc=0x5900+(4*14+1)*40,vsc=0x5900+40;void *gpu_address=(void*)0x202500000ull;
 setp(actual_buffer,0x30,device);setu(actual_buffer,0x38,5);setu(actual_buffer,0x3c,101);
 setp(actual_buffer,0x40,backing);setp(actual_buffer,0x140,actual_record);setp(actual_buffer,0x148,gpu_address);
 setu(actual_buffer,0x70,1664);setu(actual_buffer,0x74,2);setu(actual_buffer,0x78,4);setu(actual_buffer,0x7c,0x10000);
 setp(actual_record,0x20,actual_payload);setp(actual_record,0x28,gpu_address);setp(actual_record,0x30,backing);
 memset(context+psc,0,40);memset(context+vsc,0,40);
 setp(context,psc,actual_buffer);setp(context,vsc,actual_buffer);
 setp(context,psc+24,gpu_address);setp(context,psc+32,gpu_address);setp(context,vsc+24,gpu_address);setp(context,vsc+32,gpu_address);
 context[0xc48]=0;
 CHECK(FM5ColorObserverTestActualBound(context,device,bound_json,sizeof(bound_json)));
 CHECK(strstr(bound_json,"resource140_current_allocation_matching_VS_PS_prepared_CBVs")&&strstr(bound_json,"\"bytes\":\"626262"));
 CHECK(fopen_s(&post_sample,"actual-bound-cpu-sample.json","wb")==0);
 fwrite(bound_json,1,strlen(bound_json),post_sample);fclose(post_sample);
 /* Each changed identity/range/address fails closed instead of substituting last-upload bytes. */
 const SIZE_T buffer_guards[]={0x30,0x38,0x70,0x74,0x78,0x7c,0x140,0x148};
 for(UINT n=0;n<_countof(buffer_guards);++n){SIZE_T at=buffer_guards[n];actual_buffer[at]^=(BYTE)(at==0x70?0x80:1);
  CHECK(FM5ColorObserverTestActualBound(context,device,bound_json,sizeof(bound_json)));
  CHECK(strstr(bound_json,"\"actual_cpu_bound_bytes\":null"));actual_buffer[at]^=(BYTE)(at==0x70?0x80:1);
 }
 const SIZE_T entry_guards[]={0,8,12,24,32};
 for(UINT n=0;n<_countof(entry_guards);++n)for(UINT stage_index=0;stage_index<2;++stage_index){
  SIZE_T at=(stage_index?psc:vsc)+entry_guards[n];context[at]^=1;
  CHECK(FM5ColorObserverTestActualBound(context,device,bound_json,sizeof(bound_json)));
  CHECK(strstr(bound_json,"\"actual_cpu_bound_bytes\":null"));context[at]^=1;
 }
 actual_record[0x28]^=1;FM5ColorObserverTestActualBound(context,device,bound_json,sizeof(bound_json));CHECK(strstr(bound_json,"allocation_GPU_VA_does_not_match"));actual_record[0x28]^=1;
 setp(actual_record,0x20,(void*)1);FM5ColorObserverTestActualBound(context,device,bound_json,sizeof(bound_json));CHECK(strstr(bound_json,"allocation_CPU_payload_unreadable"));
 setp(actual_record,0x20,actual_payload);context[0xc48]=1;
 FM5ColorObserverTestActualBound(context,device,bound_json,sizeof(bound_json));CHECK(strstr(bound_json,"not_prepared_immediate_context"));
#ifndef FM5_COLOR_CPU_ONLY
 physical->Release();gpu->Release();
 printf("{\"checks\":%u,\"real_d3d12_getdesc\":true,\"gpu_submissions\":0,\"game_executed\":false}\n",checks);
#else
 printf("{\"checks\":%u,\"real_d3d12_getdesc\":false,\"gpu_device_created\":false,\"game_executed\":false}\n",checks);
#endif
 return 0;
}
