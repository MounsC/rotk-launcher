#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#undef NDEBUG
#include <assert.h>
static int log_count;
static void proxy_trace_line(const char *message) {
    assert(strstr(message, "unsupported HUD code") != NULL);
    log_count++;
}
#include "../voice_rank_patch.h"

int main(int argc, char **argv) {
    unsigned char *memory = VirtualAlloc(NULL, 4096, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
    assert(memory != NULL);
    const size_t length = sizeof(voice_rank_original);
    assert(length == 106);
    for (size_t index = 0; index < length; index++) {
        memcpy(memory, voice_rank_original, length);
        memory[index] ^= 1;
        unsigned char before[sizeof(voice_rank_original)];
        memcpy(before, memory, length);
        assert(voice_rank_patch_code(memory) == -1);
        assert(memcmp(before, memory, length) == 0);
    }
    memcpy(memory, voice_rank_original, length);
    memory[VOICE_RANK_TIER_BRANCH] = 0xeb;
    assert(voice_rank_patch_code(memory) == -1);
    assert(memory[VOICE_RANK_SUBTIER_BRANCH] == 0x74);
    memcpy(memory, voice_rank_original, length);
    DWORD ignored;
    assert(VirtualProtect(memory, 4096, PAGE_EXECUTE_READ, &ignored));
    assert(voice_rank_patch_code(memory) == 1);
    assert(voice_rank_patch_code(memory) == 1);
    MEMORY_BASIC_INFORMATION info;
    assert(VirtualQuery(memory, &info, sizeof(info)) == sizeof(info));
    assert(info.Protect == PAGE_EXECUTE_READ);
    for (size_t i = 0; i < length; i++) {
        assert(memory[i] == ((i == VOICE_RANK_TIER_BRANCH || i == VOICE_RANK_SUBTIER_BRANCH)
            ? 0xeb : voice_rank_original[i]));
    }
    if (argc == 2) {
        FILE *output = fopen(argv[1], "wb");
        assert(output != NULL);
        assert(fwrite(memory, 1, length, output) == length);
        assert(fclose(output) == 0);
    }
    voice_rank_ensure_initialized();
    voice_rank_ensure_initialized();
    assert(log_count == 1); /* Unsupported host stays untouched; init is once. */
    assert(VirtualFree(memory, 0, MEM_RELEASE));
    puts("PASS voice HUD patch: 106 corruptions refused, partial patch refused, exactly two code bytes changed, idempotence, protection restored, unsupported host skipped once.");
    return 0;
}
