#define DirectInput8Create RotkTestDirectInput8Create
#define DllCanUnloadNow RotkTestDllCanUnloadNow
#define DllGetClassObject RotkTestDllGetClassObject
#define DllRegisterServer RotkTestDllRegisterServer
#define DllUnregisterServer RotkTestDllUnregisterServer
#define GetdfDIJoystick RotkTestGetdfDIJoystick
#define DllMain RotkTestDllMain
#include "../dinput8_proxy.c"

#include <stdio.h>

static int write_marker(const WCHAR *path, const char *contents) {
    FILE *stream = _wfopen(path, L"wb");
    size_t length = strlen(contents);
    if (stream == NULL) {
        return 1;
    }
    if (fwrite(contents, 1U, length, stream) != length) {
        fclose(stream);
        return 1;
    }
    fclose(stream);
    return 0;
}

int main(void) {
    WCHAR path[MAX_PATH];
    int failures = 0;
    static const char good[] =
        "mode=anti-slow-v3\n"
        "patch=1046F98:8f>82,1046FE5:74>eb\n"
        "h1z1Sha256=5F5A4922B0671E4ED8FD415E753BE096EF7A17E360AE80E025F11544C8DB9261\n";
    static const char wrong_mode[] =
        "mode=anti-slow-v2\n"
        "patch=1046F98:8f>82,1046FE5:74>eb\n";
    static const char wrong_patch[] =
        "mode=anti-slow-v3\n"
        "patch=1046F98:8f>82\n";

    g_self_module = GetModuleHandleW(NULL);
    if (!module_sibling_path(path, (DWORD)ARRAYSIZE(path), k_marker_name)) {
        fputs("could not resolve marker path\n", stderr);
        return 2;
    }
    (void)_wremove(path);
    if (marker_enabled()) {
        fputs("absent marker must not enable the patch\n", stderr);
        failures += 1;
    }
    if (write_marker(path, good) != 0) {
        fputs("could not write good marker\n", stderr);
        return 2;
    }
    if (!marker_enabled()) {
        fputs("good marker must enable the patch\n", stderr);
        failures += 1;
    }
    if (write_marker(path, wrong_mode) != 0 || marker_enabled()) {
        fputs("wrong mode marker must not enable the patch\n", stderr);
        failures += 1;
    }
    if (write_marker(path, wrong_patch) != 0 || marker_enabled()) {
        fputs("wrong patch marker must not enable the patch\n", stderr);
        failures += 1;
    }
    (void)_wremove(path);
    if (marker_enabled()) {
        fputs("removed marker must disable the patch\n", stderr);
        failures += 1;
    }
    if (failures != 0) {
        return 1;
    }
    puts("marker opt-in matrix passed");
    return 0;
}
