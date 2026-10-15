#include "fm5_guest_tls.h"
#include <intrin.h>
#include <stdio.h>
#include <string.h>

#ifdef FM5_TLS_FIXTURE
__declspec(thread) unsigned char fixture_tls[64];
extern unsigned long _tls_index;
__declspec(dllexport) DWORD FM5TestTLSIndex(void) { return _tls_index; }
__declspec(dllexport) void *FM5TestTLSAddress(void) {
    return ((void **)(uintptr_t)__readgsqword(0x58))[_tls_index];
}
#else
typedef DWORD (*GetIndex)(void);
typedef void *(*GetAddress)(void);
typedef void (*Run)(uint64_t *);
static GetAddress get_address;
static Run run_sites[4];
static volatile LONG checks;
#define CHECK(x) do {if (!(x)) {fprintf(stderr,"FAIL %d: %s\n",__LINE__,#x);return 1;} InterlockedIncrement(&checks);} while (0)

static DWORD WINAPI worker(void *unused) {
    uint64_t outputs[5], value = 0xabcdef0088776655ull;
    unsigned char host_before[40];
    void **vector = (void **)(uintptr_t)__readgsqword(0x58);
    unsigned char *guest = get_address();
    unsigned int i;
    (void)unused;
    memcpy(host_before, vector[0], sizeof(host_before));
    memcpy(guest + 32, &value, 8);
    for (i = 0; i < 4; ++i) {
        memset(outputs, 0, sizeof(outputs));
        run_sites[i](outputs);
        CHECK(outputs[0] == value);
        CHECK(outputs[1] == outputs[3]);
        CHECK(outputs[2] == 0x1122334455667788ull);
        CHECK(!memcmp(host_before, vector[0], sizeof(host_before)));
        if (i >= 2) CHECK(outputs[4] == (uint64_t)(uintptr_t)vector);
    }
    return 0;
}

int wmain(int argc, wchar_t **argv) {
    HMODULE module;
    DWORD index, protection, exit_code;
    GetIndex get_index;
    HANDLE thread;
    unsigned char *image;
    unsigned int i;
    FM5TLSRedirectSite sites[4];
    FM5TLSRedirect redirect;
    MEMORY_BASIC_INFORMATION memory;
    const unsigned char prefix[] = {
        0x56,0x48,0x89,0xce,
        0x49,0xbb,0x88,0x77,0x66,0x55,0x44,0x33,0x22,0x11,
        0x31,0xc0,0xf9,0x9c,0x41,0x5a,0x4c,0x89,0x56,0x18
    };
    const unsigned char suffix[] = {
        0x48,0x8b,0x00,0x48,0x8b,0x40,0x20,0x48,0x89,0x06,
        0x9c,0x58,0x48,0x89,0x46,0x08,0x4c,0x89,0x5e,0x10,0x5e,0xc3
    };
    CHECK(argc == 2);
    module = LoadLibraryW(argv[1]);
    CHECK(module != NULL);
    get_index = (GetIndex)(uintptr_t)GetProcAddress(module,"FM5TestTLSIndex");
    get_address = (GetAddress)(uintptr_t)GetProcAddress(module,"FM5TestTLSAddress");
    CHECK(get_index && get_address);
    index = get_index();
    CHECK(index != 0);
    image = VirtualAlloc(NULL, 4096, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    CHECK(image != NULL);
    memset(sites, 0, sizeof(sites));
    for (i = 0; i < 4; ++i) {
        unsigned char *start = image + i * 128;
        unsigned char instruction[] = {0x65,0x48,0x8b,0x04,0x25,0x58,0,0,0};
        if (i == 1) instruction[3] = 0x0c;
        memcpy(start, prefix, sizeof(prefix));
        memcpy(start + sizeof(prefix), instruction, 9);
        if (i < 2) {
            memcpy(start + sizeof(prefix) + 9, suffix, sizeof(suffix));
            if (i) start[sizeof(prefix) + 9 + 2] = 0x01;
        } else {
            unsigned char special[] = {
                0x48,0x8b,0x08,
                0x48,0x89,0x46,0x20,
                0x48,0x8b,0x41,0x20
            };
            if (i == 3) {special[2] = 0x10; special[9] = 0x42;}
            memcpy(start + sizeof(prefix) + 9, special, sizeof(special));
            memcpy(start + sizeof(prefix) + 9 + sizeof(special), suffix + 7, sizeof(suffix) - 7);
        }
        sites[i].rva = i * 128 + sizeof(prefix);
        sites[i].destination = i == 1 ? 1 : 0;
        sites[i].patch_bytes = i < 2 ? 9 : 12;
        sites[i].lookup_destination = i < 2 ? 0xff : (uint8_t)(i - 1);
        memcpy(sites[i].expected, start + sizeof(prefix), sites[i].patch_bytes);
        run_sites[i] = (Run)(uintptr_t)start;
    }
    CHECK(FM5RedirectGuestTLS(NULL,4096,index,sites,2,&redirect) == E_POINTER);
    CHECK(FM5RedirectGuestTLS(image,4096,0,sites,2,&redirect) == E_INVALIDARG);
    CHECK(FM5RedirectGuestTLS(image,4096,65536,sites,2,&redirect) == E_INVALIDARG);
    sites[1].rva = sites[0].rva;
    sites[1].destination = 0;
    memcpy(sites[1].expected, sites[0].expected,9);
    CHECK(FM5RedirectGuestTLS(image,4096,index,sites,2,&redirect) == E_INVALIDARG);
    sites[1].rva = 128 + sizeof(prefix);
    sites[1].destination = 1;
    sites[1].expected[3] = 0x0c;
    sites[1].expected[0] = 0x90;
    CHECK(FAILED(FM5RedirectGuestTLS(image,4096,index,sites,2,&redirect)));
    CHECK(image[sites[0].rva] == 0x65);
    sites[1].expected[0] = 0x65;
    CHECK(VirtualProtect(image,4096,PAGE_EXECUTE_READ,&protection));
    CHECK(SUCCEEDED(FM5RedirectGuestTLS(image,4096,index,sites,4,&redirect)));
    CHECK(redirect.count == 4);
    CHECK(VirtualQuery(redirect.code,&memory,sizeof(memory)));
    CHECK(memory.Protect == PAGE_EXECUTE_READ);
    CHECK(VirtualQuery(image,&memory,sizeof(memory)));
    CHECK(memory.Protect == PAGE_EXECUTE_READ);
    CHECK(worker(NULL) == 0);
    thread = CreateThread(NULL,0,worker,NULL,0,NULL);
    CHECK(thread != NULL);
    CHECK(WaitForSingleObject(thread,5000) == WAIT_OBJECT_0);
    CHECK(GetExitCodeThread(thread,&exit_code) && exit_code == 0);
    CloseHandle(thread);
    VirtualFree(image,0,MEM_RELEASE);
    VirtualFree(redirect.code,0,MEM_RELEASE);
    FreeLibrary(module);
    printf("PASS: %ld native TLS checks, two registers, two threads, flags and host storage preserved\n",checks);
    return 0;
}
#endif
