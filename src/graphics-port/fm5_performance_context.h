#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

typedef void (*FM5ContextDrawObserver)(void *native_context,BOOL after);
/* Configure before guest execution; observer never changes draw arguments. */
void FM5ConfigureContextDrawObserver(FM5ContextDrawObserver observer);
typedef void (*FM5ContextMapObserver)(void *,void *,UINT,UINT,HRESULT,const void *);
typedef void (*FM5ContextUnmapObserver)(void *,void *,UINT);
void FM5ConfigureContextMapObserver(FM5ContextMapObserver mapped,FM5ContextUnmapObserver unmapping);

typedef HRESULT (*FM5AutoCompletionPolicy)(void *native_context, BOOL enabled);
typedef HRESULT (*FM5TopologyPolicy)(void *native_context, UINT topology);
typedef HRESULT (*FM5DeviceWrapper)(void *native_owned, void **result);
/* Returns the legacy 48-byte Texture2D descriptor only after validating kind. */
typedef HRESULT (WINAPI *FM5Texture2DDescriptionPolicy)(void *resource, void *description48);
/* Snapshot contains up to16 actual native RECTs, after required CPU ordering. */
typedef HRESULT (WINAPI *FM5ScissorSnapshotPolicy)(void *native_context, UINT *count, void *rectangles256);
typedef HRESULT (WINAPI *FM5PixelSamplersPolicy)(void *native_context,UINT start,UINT count,void **samplers);

HRESULT FM5WrapPerformanceContext(void *native_owned, void **result);
void FM5ConfigurePerformanceContext(FM5AutoCompletionPolicy policy);
void FM5ConfigureContextInterop(FM5TopologyPolicy topology, FM5DeviceWrapper device_wrapper);
void FM5ConfigureContextResolve(FM5Texture2DDescriptionPolicy description);
void FM5ConfigureContextScissors(FM5ScissorSnapshotPolicy snapshot);
void FM5ConfigureContextSamplers(FM5PixelSamplersPolicy getter);
void *FM5UnwrapPerformanceContext(void *context);
