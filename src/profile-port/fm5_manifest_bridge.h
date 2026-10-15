#ifndef FM5_MANIFEST_BRIDGE_H
#define FM5_MANIFEST_BRIDGE_H

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stddef.h>

typedef struct FM5XmlSpan {
    const char *data;
    size_t length;
} FM5XmlSpan;

typedef void *(*FM5ManifestCollector)(void *, const FM5XmlSpan *, const FM5XmlSpan *);

int FM5SelectNetworkTemplate(const FM5XmlSpan *xml, FM5XmlSpan *selected);
__declspec(dllexport) void *FM5CollectNetworkTemplate(
    void *output, const FM5XmlSpan *xml, const FM5XmlSpan *tag);

#ifdef FM5_BRIDGE_TEST
void FM5ManifestSetTestCollector(FM5ManifestCollector collector);
#endif

#endif
