#define DirectInput8Create RotkTestDirectInput8Create
#define DllCanUnloadNow RotkTestDllCanUnloadNow
#define DllGetClassObject RotkTestDllGetClassObject
#define DllRegisterServer RotkTestDllRegisterServer
#define DllUnregisterServer RotkTestDllUnregisterServer
#define GetdfDIJoystick RotkTestGetdfDIJoystick
#define DllMain RotkTestDllMain
#include "../dinput8_proxy.c"

#include <stdio.h>

static int require_result(BOOL actual, BOOL expected, const char *label) {
    if (!!actual != !!expected) {
        fprintf(stderr, "%s: got %d, expected %d\n", label, !!actual, !!expected);
        return 1;
    }
    return 0;
}

int main(void) {
    SYSTEM_INFO information;
    BYTE *page;
    DWORD ignored;
    int failures = 0;

    GetSystemInfo(&information);
    page = (BYTE *)VirtualAlloc(
        NULL,
        information.dwPageSize,
        MEM_RESERVE | MEM_COMMIT,
        PAGE_READWRITE);
    if (page == NULL) {
        fputs("VirtualAlloc failed\n", stderr);
        return 2;
    }

    VirtualProtect(page, 1U, PAGE_EXECUTE_READ, &ignored);
    failures += require_result(
        executable_committed_range(page, 1U, FALSE), TRUE, "RX accepted");
    failures += require_result(
        executable_committed_range(page, 1U, TRUE), TRUE, "RX nonwrite accepted");

    VirtualProtect(page, 1U, PAGE_EXECUTE_READWRITE, &ignored);
    failures += require_result(
        executable_committed_range(page, 1U, FALSE), TRUE, "RWX accepted");
    failures += require_result(
        executable_committed_range(page, 1U, TRUE), FALSE, "RWX nonwrite refused");

    VirtualProtect(page, 1U, PAGE_READWRITE, &ignored);
    failures += require_result(
        executable_committed_range(page, 1U, FALSE), FALSE, "RW refused");

    VirtualProtect(page, 1U, PAGE_NOACCESS, &ignored);
    failures += require_result(
        executable_committed_range(page, 1U, FALSE), FALSE, "NOACCESS refused");

    VirtualProtect(page, 1U, PAGE_EXECUTE_READ | PAGE_GUARD, &ignored);
    failures += require_result(
        executable_committed_range(page, 1U, FALSE), FALSE, "GUARD refused");

    VirtualFree(page, 0U, MEM_RELEASE);
    if (failures != 0) {
        return 1;
    }
    puts("target protection matrix passed (RX/RWX accepted; non-exec/guard refused)");
    return 0;
}
