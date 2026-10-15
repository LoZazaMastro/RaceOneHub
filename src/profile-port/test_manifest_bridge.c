#include "fm5_manifest_bridge.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char tag_text[] = "mx:SecureDeviceAssociationTemplate";
static const char *original_xml;
static size_t calls;

static void *mock_collector(void *output, const FM5XmlSpan *span, const FM5XmlSpan *tag) {
    uintptr_t *words = (uintptr_t *)output;
    if (span->data != original_xml + 7157 || span->length != 394 ||
        tag->length != sizeof(tag_text) - 1 || memcmp(tag->data, tag_text, tag->length)) abort();
    ++calls;
    words[0] = 0x11223344u;
    words[1] = 0x55667788u;
    words[2] = 0x99aabbccu;
    return output;
}

int wmain(int argc, wchar_t **argv) {
    FILE *file;
    char *data, *copy;
    long bytes;
    FM5XmlSpan xml, selected, tag = {tag_text, sizeof(tag_text) - 1};
    uintptr_t output[3] = {0, 0, 0};
    size_t selected_offset;
    int tests = 0;
#define CHECK(expression) do { if (!(expression)) { fprintf(stderr, "failed line %d\n", __LINE__); return 1; } ++tests; } while (0)
    CHECK(argc == 2);
    CHECK(_wfopen_s(&file, argv[1], L"rb") == 0);
    CHECK(fseek(file, 0, SEEK_END) == 0);
    bytes = ftell(file);
    CHECK(bytes == 11477);
    rewind(file);
    data = (char *)malloc((size_t)bytes);
    copy = (char *)malloc((size_t)bytes);
    CHECK(data && copy);
    CHECK(fread(data, 1, (size_t)bytes, file) == (size_t)bytes);
    fclose(file);
    xml.data = data; xml.length = (size_t)bytes;
    original_xml = data;
    CHECK(FM5SelectNetworkTemplate(&xml, &selected));
    CHECK(selected.data == data + 7157 && selected.length == 394);
    selected_offset = (size_t)(selected.data - data);
    memcpy(copy, data, (size_t)bytes);
    FM5ManifestSetTestCollector(mock_collector);
    CHECK(FM5CollectNetworkTemplate(output, &xml, &tag) == output);
    CHECK(calls == 1 && output[0] == 0x11223344u && output[1] == 0x55667788u && output[2] == 0x99aabbccu);
    CHECK(memcmp(copy, data, (size_t)bytes) == 0);
    data[selected_offset + 90] ^= 1;
    CHECK(!FM5SelectNetworkTemplate(&xml, &selected));
    CHECK(!selected.data && !selected.length);
    memcpy(data, copy, (size_t)bytes);
    {
        const char *socket = strstr(data, "Name=\"SecureServerSocket\" SecureIpProtocol=\"Udp\" BoundPort=\"5100\"");
        size_t offset;
        CHECK(socket != NULL);
        offset = (size_t)(socket - data) + strlen("Name=\"SecureServerSocket\" SecureIpProtocol=\"Udp\" BoundPort=\"");
        data[offset] = '6';
        CHECK(!FM5SelectNetworkTemplate(&xml, &selected));
        memcpy(data, copy, (size_t)bytes);
    }
    xml.length -= 1;
    CHECK(!FM5SelectNetworkTemplate(&xml, &selected));
    xml.length += 1;
    memcpy(data, "Name=\"SecurePeerTraffic\"", 24);
    CHECK(!FM5SelectNetworkTemplate(&xml, &selected));
    memcpy(data, copy, (size_t)bytes);
    CHECK(!FM5SelectNetworkTemplate(NULL, &selected));
    CHECK(!FM5SelectNetworkTemplate(&xml, NULL));
    tag.length -= 1;
    __try { FM5CollectNetworkTemplate(output, &xml, &tag); CHECK(0); }
    __except (GetExceptionCode() == 0xe0464d35u ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) { ++tests; }
    CHECK(calls == 1);
    free(data); free(copy);
    printf("{\"passed\":true,\"checks\":%d,\"collector_calls\":%zu,\"selected_start\":7157,\"selected_bytes\":394}\n", tests, calls);
    return 0;
}
