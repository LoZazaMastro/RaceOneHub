#pragma once
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#define FM5_PRIVATE_NOT_FOUND ((HRESULT)0x88790002)
HRESULT WINAPI FM5GetPrivateData(void *object,REFGUID key,UINT *bytes,void *data);
HRESULT WINAPI FM5SetPrivateData(void *object,REFGUID key,UINT bytes,const void *data);
HRESULT WINAPI FM5SetPrivateDataInterface(void *object,REFGUID key,void *value);
HRESULT WINAPI FM5SetPrivateDataInterface2(void *object,REFGUID key,void *value);
void WINAPI FM5ForgetPrivateData(void *object);
#ifdef FM5_PRIVATE_DATA_TEST
void FM5PrivateDataFailAllocation(LONG countdown);
void FM5PrivateDataCounts(SIZE_T *objects,SIZE_T *properties);
#endif
