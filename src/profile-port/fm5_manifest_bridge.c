#include "fm5_manifest_bridge.h"
#include "secure_template.h"
#include <stdint.h>
#include <string.h>

#ifdef FM5_BRIDGE_TEST
static FM5ManifestCollector test_collector;
void FM5ManifestSetTestCollector(FM5ManifestCollector collector) { test_collector = collector; }
#endif

static size_t count_bytes(const FM5XmlSpan *source, const char *needle, size_t length) {
    size_t index, count = 0;
    if (!length || length > source->length) return 0;
    for (index = 0; index <= source->length - length; ++index)
        if (!memcmp(source->data + index, needle, length)) ++count;
    return count;
}

int FM5SelectNetworkTemplate(const FM5XmlSpan *xml, FM5XmlSpan *selected) {
    static const char name[] = "Name=\"SecurePeerTraffic\"";
    static const char open_tag[] = "<mx:SecureDeviceAssociationTemplate ";
    static const char server_socket[] = "<mx:SocketDescription Name=\"SecureServerSocket\" SecureIpProtocol=\"Udp\" BoundPort=\"5100\">";
    static const char console_socket[] = "<mx:SocketDescription Name=\"SecureConsoleSocket\" SecureIpProtocol=\"Udp\" BoundPort=\"5100\">";
    size_t index;
    if (!selected) return 0;
    selected->data = NULL;
    selected->length = 0;
    if (!xml || !xml->data || xml->length != FM5_MANIFEST_BYTES ||
        count_bytes(xml, name, sizeof(name) - 1) != 1 ||
        count_bytes(xml, open_tag, sizeof(open_tag) - 1) != 5 ||
        count_bytes(xml, server_socket, sizeof(server_socket) - 1) != 1 ||
        count_bytes(xml, console_socket, sizeof(console_socket) - 1) != 1) return 0;
    for (index = 0; index <= xml->length - (sizeof(fm5_secure_template) - 1); ++index) {
        if (!memcmp(xml->data + index, fm5_secure_template, sizeof(fm5_secure_template) - 1)) {
            selected->data = xml->data + index;
            selected->length = sizeof(fm5_secure_template) - 1;
            return 1;
        }
    }
    return 0;
}

static FM5ManifestCollector collector(void) {
#ifdef FM5_BRIDGE_TEST
    return test_collector;
#else
    static const unsigned char entry[] = {
        0x48,0x89,0x5c,0x24,0x10,0x4c,0x89,0x44,0x24,0x18,
        0x48,0x89,0x4c,0x24,0x08,0x55,0x56,0x57,0x41,0x54,0x41,0x55,0x41,0x56
    };
    unsigned char *host = (unsigned char *)GetModuleHandleW(NULL);
    IMAGE_DOS_HEADER *dos;
    IMAGE_NT_HEADERS64 *nt;
    if (!host) return NULL;
    dos = (IMAGE_DOS_HEADER *)host;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE || dos->e_lfanew < 0x40 || dos->e_lfanew > 0x1000) return NULL;
    nt = (IMAGE_NT_HEADERS64 *)(host + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE || nt->FileHeader.Machine != IMAGE_FILE_MACHINE_AMD64 ||
        nt->OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC ||
        nt->OptionalHeader.SizeOfImage < 0x841b0 + sizeof(entry) ||
        memcmp(host + 0x841b0, entry, sizeof(entry))) return NULL;
    return (FM5ManifestCollector)(host + 0x841b0);
#endif
}

void *FM5CollectNetworkTemplate(void *output, const FM5XmlSpan *xml, const FM5XmlSpan *tag) {
    static const char expected_tag[] = "mx:SecureDeviceAssociationTemplate";
    FM5XmlSpan selected;
    FM5ManifestCollector original;
    if (output && tag && tag->data && tag->length == sizeof(expected_tag) - 1 &&
        !memcmp(tag->data, expected_tag, sizeof(expected_tag) - 1) &&
        FM5SelectNetworkTemplate(xml, &selected) && (original = collector()) != NULL)
        return original(output, &selected, tag);
    SetLastError(ERROR_INVALID_DATA);
    RaiseException(0xe0464d35u, EXCEPTION_NONCONTINUABLE, 0, NULL);
    return NULL;
}
