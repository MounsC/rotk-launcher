#ifndef ROTK_VOICE_RANK_PATCH_H
#define ROTK_VOICE_RANK_PATCH_H

/* BR1315 VoiceRecentParticipants Tier/Subtier must retain Voice.Identity.
 * The retail getters overwrite the row with the actor's rank, including the
 * local/lightweight actor's zero. Only the two conditional branches change;
 * names, audio, actor state and the kill-feed data source are untouched.
 * Executed at the first Vivox poll, after the client has unpacked its code.
 */
#define VOICE_RANK_RVA 0x01ae9d4eU
#define VOICE_RANK_TIER_BRANCH 29U
#define VOICE_RANK_SUBTIER_BRANCH 82U
static const unsigned char voice_rank_original[] = {
    0x48,0x8b,0x47,0x10,0x48,0x89,0x44,0x24,0x20,0x48,0x8d,0x54,0x24,0x20,0x48,0x8b,
    0x0d,0x2d,0x35,0xc8,0x02,0xe8,0x1b,0x7c,0x51,0xfe,0x48,0x85,0xc0,0x74,0x0d,0x33,
    0xd2,0x48,0x8b,0xc8,0xe8,0x68,0x60,0x52,0xfe,0x89,0x47,0x78,0x44,0x8b,0x47,0x78,
    0xe9,0xd0,0xfe,0xff,0xff,0x48,0x8b,0x47,0x10,0x48,0x89,0x44,0x24,0x28,0x48,0x8d,
    0x54,0x24,0x28,0x48,0x8b,0x0d,0xf8,0x34,0xc8,0x02,0xe8,0xe6,0x7b,0x51,0xfe,0x48,
    0x85,0xc0,0x74,0x0d,0x33,0xd2,0x48,0x8b,0xc8,0xe8,0x33,0xcb,0x5a,0xfe,0x89,0x47,
    0x7c,0x44,0x8b,0x47,0x7c,0xe9,0x9b,0xfe,0xff,0xff
};

/* 0: original, 1: already installed, -1: unknown (including partial patches). */
static int voice_rank_code_state(const unsigned char *code) {
    unsigned char normalized[sizeof(voice_rank_original)];
    int installed = code[VOICE_RANK_TIER_BRANCH] == 0xeb &&
                    code[VOICE_RANK_SUBTIER_BRANCH] == 0xeb;
    memcpy(normalized, code, sizeof(normalized));
    if (installed) {
        normalized[VOICE_RANK_TIER_BRANCH] = 0x74;
        normalized[VOICE_RANK_SUBTIER_BRANCH] = 0x74;
    }
    return memcmp(normalized, voice_rank_original, sizeof(normalized)) == 0
        ? installed : -1;
}

static int voice_rank_patch_code(unsigned char *code) {
    DWORD old_protection, ignored;
    int state = voice_rank_code_state(code);
    if (state != 0) return state;
    if (!VirtualProtect(code, sizeof(voice_rank_original), PAGE_EXECUTE_READWRITE,
                        &old_protection)) return -2;
    code[VOICE_RANK_TIER_BRANCH] = 0xeb;
    code[VOICE_RANK_SUBTIER_BRANCH] = 0xeb;
    BOOL flushed = FlushInstructionCache(GetCurrentProcess(), code,
                                         sizeof(voice_rank_original));
    BOOL restored = VirtualProtect(code, sizeof(voice_rank_original),
                                    old_protection, &ignored);
    return flushed && restored ? 1 : -2;
}

static BOOL CALLBACK voice_rank_initialize(PINIT_ONCE once, PVOID parameter,
                                           PVOID *context) {
    (void)once; (void)parameter; (void)context;
    unsigned char *base = (unsigned char *)GetModuleHandleW(NULL);
    MEMORY_BASIC_INFORMATION memory;
    int result = -1;
    if (base != NULL) {
        IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
        if (dos->e_magic == IMAGE_DOS_SIGNATURE && dos->e_lfanew > 0 &&
            dos->e_lfanew < 0x100000) {
            IMAGE_NT_HEADERS64 *nt = (IMAGE_NT_HEADERS64 *)(base + dos->e_lfanew);
            if (nt->Signature == IMAGE_NT_SIGNATURE &&
                nt->FileHeader.Machine == IMAGE_FILE_MACHINE_AMD64 &&
                nt->OptionalHeader.Magic == IMAGE_NT_OPTIONAL_HDR64_MAGIC &&
                nt->OptionalHeader.SizeOfImage >= VOICE_RANK_RVA + sizeof(voice_rank_original)) {
                unsigned char *code = base + VOICE_RANK_RVA;
                if (VirtualQuery(code, &memory, sizeof(memory)) == sizeof(memory) &&
                    memory.State == MEM_COMMIT && memory.Type == MEM_IMAGE &&
                    !(memory.Protect & (PAGE_GUARD | PAGE_NOACCESS)) &&
                    (memory.Protect & (PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE |
                                       PAGE_EXECUTE_WRITECOPY)) &&
                    (uintptr_t)memory.BaseAddress + memory.RegionSize >=
                        (uintptr_t)code + sizeof(voice_rank_original)) {
                    result = voice_rank_patch_code(code);
                }
            }
        }
    }
    proxy_trace_line(result == 1
        ? "[voice-rank] Voice.Identity HUD source installed"
        : result == -2 ? "[voice-rank] HUD patch protection/cache failure"
        : "[voice-rank] unsupported HUD code; voice audio remains available");
    return TRUE;
}

static INIT_ONCE voice_rank_once = INIT_ONCE_STATIC_INIT;
static void voice_rank_ensure_initialized(void) {
    (void)InitOnceExecuteOnce(&voice_rank_once, voice_rank_initialize, NULL, NULL);
}
#endif
