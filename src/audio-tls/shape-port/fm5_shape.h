#ifndef FM5_SHAPE_H
#define FM5_SHAPE_H
#include <windows.h>
#include <stdint.h>

/* The original queue owns job and its graph. This synchronous boundary borrows both. */
__declspec(dllexport) BOOLEAN WINAPI FM5ExecuteShapeGraph(void *hal, const void *legacy_job);
HRESULT FM5TranslateShapeGraph(UINT64 *destination, const UINT64 *source, SIZE_T bytes);
#ifdef FM5_SHAPE_TEST
typedef BOOLEAN (WINAPI *FM5ShapeExecutor)(void *, const void *);
typedef void *(*FM5ShapeAllocator)(SIZE_T);
typedef void (*FM5ShapeDeallocator)(void *);
void FM5ShapeSetTestBackends(FM5ShapeExecutor, FM5ShapeAllocator, FM5ShapeDeallocator);
#endif
#endif
