#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>

typedef HRESULT(WINAPI *direct_input8_create_fn)(
    HINSTANCE, DWORD, const GUID *, void **, void *);
typedef HRESULT(WINAPI *hresult_no_args_fn)(void);
typedef const void *(WINAPI *get_joystick_format_fn)(void);
typedef ULONG(WINAPI *release_fn)(void *);

static const GUID k_iid_direct_input8_w = {
    0xBF798031, 0x483A, 0x4DA2,
    {0xAA, 0x99, 0x5D, 0x64, 0xED, 0x36, 0x97, 0x00},
};

static FARPROC require_export(HMODULE module, const char *name) {
    FARPROC address = GetProcAddress(module, name);
    if (address == NULL) {
        fprintf(stderr, "missing export: %s\n", name);
        ExitProcess(2U);
    }
    return address;
}

static FARPROC require_ordinal(
    HMODULE module,
    WORD ordinal,
    const char *name) {
    FARPROC by_name = require_export(module, name);
    FARPROC by_ordinal = GetProcAddress(
        module,
        (const char *)(uintptr_t)ordinal);
    if (by_ordinal == NULL || by_ordinal != by_name) {
        fprintf(stderr, "bad export ordinal %u for %s\n", ordinal, name);
        ExitProcess(2U);
    }
    return by_name;
}

int main(int argc, char **argv) {
    HMODULE proxy;
    direct_input8_create_fn direct_input8_create;
    hresult_no_args_fn can_unload;
    get_joystick_format_fn get_joystick_format;
    void *direct_input = NULL;
    HRESULT result;
    union { FARPROC source; direct_input8_create_fn destination; } direct_cast;
    union { FARPROC source; hresult_no_args_fn destination; } unload_cast;
    union { FARPROC source; get_joystick_format_fn destination; } joystick_cast;

    if (argc != 2) {
        fputs("usage: dinput8_proxy_smoke.exe <absolute-dinput8.dll>\n", stderr);
        return 2;
    }
    proxy = LoadLibraryA(argv[1]);
    if (proxy == NULL) {
        fprintf(stderr, "LoadLibrary failed: %lu\n", GetLastError());
        return 2;
    }

    direct_cast.source = require_ordinal(proxy, 1U, "DirectInput8Create");
    direct_input8_create = direct_cast.destination;
    unload_cast.source = require_ordinal(proxy, 2U, "DllCanUnloadNow");
    can_unload = unload_cast.destination;
    require_ordinal(proxy, 3U, "DllGetClassObject");
    require_ordinal(proxy, 4U, "DllRegisterServer");
    require_ordinal(proxy, 5U, "DllUnregisterServer");
    joystick_cast.source = require_ordinal(proxy, 6U, "GetdfDIJoystick");
    get_joystick_format = joystick_cast.destination;
    if (GetProcAddress(proxy, (const char *)(uintptr_t)7U) != NULL) {
        fputs("unexpected seventh ordinal export\n", stderr);
        return 2;
    }

    if (get_joystick_format() == NULL) {
        fputs("GetdfDIJoystick forwarding failed\n", stderr);
        return 3;
    }
    result = can_unload();
    if (result != S_OK && result != S_FALSE) {
        fprintf(stderr, "DllCanUnloadNow failed: 0x%08lx\n", (unsigned long)result);
        return 3;
    }
    result = direct_input8_create(
        GetModuleHandleW(NULL), 0x0800U, &k_iid_direct_input8_w,
        &direct_input, NULL);
    if (FAILED(result) || direct_input == NULL) {
        fprintf(stderr, "DirectInput8Create failed: 0x%08lx\n", (unsigned long)result);
        return 4;
    }
    {
        void **vtable = *(void ***)direct_input;
        union { void *source; release_fn destination; } release_cast;
        release_cast.source = vtable[2];
        release_cast.destination(direct_input);
    }

    puts("dinput8 proxy smoke test passed");
    return 0;
}
