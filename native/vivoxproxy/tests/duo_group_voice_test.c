/* Compile the shipping implementation so this exercises the real WinHTTP path. */
#define ROTK_VIVOX_V5_COMPAT 1
#include "../vivoxsdk_x64_proxy.c"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <assert.h>
#include <stdlib.h>

static char *test_duplicate(const char *value) {
    size_t bytes = strlen(value) + 1U;
    char *copy = malloc(bytes);
    assert(copy != NULL);
    memcpy(copy, value, bytes);
    return copy;
}

static int test_free(char *value) { free(value); return 0; }

int main(int argc, char **argv) {
    uint8_t request[SESSION_REQUEST_BYTES] = {0};
    WCHAR channel[64];
    char too_long[65];
    char *unsafe = "sip:confctl-g-test.room@domain\r\nInjected: yes";
    char *valid = "sip:confctl-g-90724-rotk-78630.g-0000000000001@mtu1xp.vivox.com";
    assert(strlen(valid) == 63U);
    write_pointer(request, SESSION_URI_OFFSET, valid);
    assert(copy_requested_channel(request, REQUEST_SESSION, channel));
    assert(wcslen(channel) == 63U);
    write_pointer(request, SESSIONGROUP_URI_OFFSET, valid);
    assert(copy_requested_channel(request, REQUEST_SESSIONGROUP_ADD, channel));
    write_pointer(request, SESSIONGROUP_URI_OFFSET, unsafe);
    assert(!copy_requested_channel(request, REQUEST_SESSIONGROUP_ADD, channel));
    memset(too_long, 'x', 64U); too_long[64] = '\0';
    memcpy(too_long, "sip:confctl-g-", 14U);
    write_pointer(request, SESSIONGROUP_URI_OFFSET, too_long);
    assert(!copy_requested_channel(request, REQUEST_SESSIONGROUP_ADD, channel));
    write_pointer(request, SESSIONGROUP_URI_OFFSET, NULL);
    assert(!copy_requested_channel(request, REQUEST_SESSIONGROUP_ADD, channel));
    assert(!copy_requested_channel(NULL, REQUEST_SESSION, channel));
    assert(!copy_requested_channel(request, 0x999U, channel));
    assert(copy_requested_channel(NULL, REQUEST_LOGIN, channel));
    assert(channel[0] == L'\0');

    if (argc == 4) {
        voice_grant grant;
        g_config.valid = TRUE;
        g_config.secure = FALSE;
        g_config.port = (INTERNET_PORT)atoi(argv[1]);
        wcscpy(g_config.host, L"127.0.0.1");
        wcscpy(g_config.session_id, L"duo-proof-token-1");
        g_strdup = test_duplicate;
        g_free = test_free;
        assert(fetch_grant(VOICE_ACTION_LOGIN, L"", &grant));
        memcpy(g_account, grant.account, strlen(grant.account) + 1U);
        for (int arg = 2; arg <= 3; ++arg) {
            for (int kind = 0; kind < 2; ++kind) {
                uint32_t type = kind == 0 ? REQUEST_SESSION : REQUEST_SESSIONGROUP_ADD;
                size_t uri_offset = kind == 0 ? SESSION_URI_OFFSET : SESSIONGROUP_URI_OFFSET;
                size_t token_offset = kind == 0 ? SESSION_TOKEN_OFFSET : SESSIONGROUP_TOKEN_OFFSET;
                size_t request_bytes = kind == 0 ? SESSION_REQUEST_BYTES : SESSIONGROUP_REQUEST_BYTES;
                char *token = NULL, *uri = NULL;
                memset(request, 0, sizeof(request));
                write_pointer(request, uri_offset, argv[arg]);
                assert(copy_requested_channel(request, type, channel));
                assert(fetch_grant(VOICE_ACTION_JOIN, channel, &grant));
                assert(strcmp(grant.channel, argv[arg]) == 0);
                assert(mutate_join(request, request_bytes, uri_offset, token_offset, &grant));
                read_pointer(request, token_offset, &token);
                read_pointer(request, uri_offset, &uri);
                assert(strcmp(token, grant.token) == 0);
                assert(uri == argv[arg]); /* Never replace Group with Proximity. */
                free(token);
                write_pointer(request, token_offset, NULL);
                grant.channel[0] = 'x';
                assert(!mutate_join(request, request_bytes, uri_offset, token_offset, &grant));
            }
        }
        puts("PASS: shipping native proxy fetches and injects distinct Group and Proximity grants over HTTP.");
    } else {
        assert(argc == 1);
        puts("PASS: native channel extraction accepts both join ABIs and rejects invalid/header-injection input.");
    }
    return 0;
}
