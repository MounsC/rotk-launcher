#ifndef ROTK_VOICE_VOLUME_COMPAT_H
#define ROTK_VOICE_VOLUME_COMPAT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>

/* BR1315's connector volume requests return NotImplemented (1017) in v5.
 * Create a real aux request: changing a legacy request's type alone skips
 * SDK factory registration and makes the shipped runtime assert internally.
 * Keep the original until delivery, preserving the game's cookies and handle.
 * All requests are created and destroyed by the SDK. Callers serialize access
 * and validate request/response memory before invoking these helpers. */
#define ROTK_VOLUME_PENDING_LIMIT 128U
#define ROTK_VOLUME_REQUEST_BYTES 0x40U
#define ROTK_VOLUME_RESPONSE_BYTES 0x40U
#define ROTK_VOLUME_SPEAKER_LEGACY 0x40U
#define ROTK_VOLUME_MIC_LEGACY 0x3fU
#define ROTK_VOLUME_SPEAKER_AUX 0x5eU
#define ROTK_VOLUME_MIC_AUX 0x5dU
typedef int (*rotk_volume_create_fn)(void **);
typedef int (*rotk_volume_destroy_fn)(void *);
typedef char *(*rotk_volume_duplicate_fn)(const char *);
typedef struct rotk_volume_pending {
    void *original;
    void *replacement;
    uint32_t legacy_type;
} rotk_volume_pending;
typedef struct rotk_volume_table {
    rotk_volume_pending entries[ROTK_VOLUME_PENDING_LIMIT];
} rotk_volume_table;

static int rotk_volume_is_legacy(uint32_t type) {
    return type == ROTK_VOLUME_SPEAKER_LEGACY || type == ROTK_VOLUME_MIC_LEGACY;
}

static void *rotk_volume_prepare(rotk_volume_table *table, void *original,
                                rotk_volume_create_fn create,
                                rotk_volume_destroy_fn destroy,
                                rotk_volume_duplicate_fn duplicate) {
    uint32_t type;
    int32_t level;
    size_t index;
    void *replacement = NULL;
    char *cookie = NULL;
    rotk_volume_pending *entry = NULL;
    memcpy(&type, (unsigned char *)original + 0x18U, sizeof(type));
    if (!rotk_volume_is_legacy(type)) return NULL;
    for (index = 0U; index < ROTK_VOLUME_PENDING_LIMIT; ++index) {
        if (table->entries[index].original == original) return NULL;
        if (entry == NULL && table->entries[index].original == NULL)
            entry = &table->entries[index];
    }
    if (entry == NULL || create(&replacement) != 0 || replacement == NULL) return NULL;
    memcpy(&cookie, (unsigned char *)original + 0x20U, sizeof(cookie));
    if (cookie != NULL) {
        cookie = duplicate(cookie);
        if (cookie == NULL) { (void)destroy(replacement); return NULL; }
        memcpy((unsigned char *)replacement + 0x20U, &cookie, sizeof(cookie));
    }
    /* vcookie is opaque caller data, never owned by the SDK. */
    memcpy((unsigned char *)replacement + 0x28U, (unsigned char *)original + 0x28U, sizeof(void *));
    memcpy(&level, (unsigned char *)original + 0x38U, sizeof(level));
    memcpy((unsigned char *)replacement + 0x30U, &level, sizeof(level));
    entry->original = original;
    entry->replacement = replacement;
    entry->legacy_type = type;
    return replacement;
}

static void rotk_volume_cancel(rotk_volume_table *table, void *replacement,
                              rotk_volume_destroy_fn destroy) {
    for (size_t index = 0U; index < ROTK_VOLUME_PENDING_LIMIT; ++index) {
        rotk_volume_pending *entry = &table->entries[index];
        if (entry->replacement == replacement && replacement != NULL) {
            (void)destroy(replacement);
            memset(entry, 0, sizeof(*entry));
            return;
        }
    }
}

static void rotk_volume_restore_response(rotk_volume_table *table, void *message,
                                        rotk_volume_destroy_fn destroy) {
    uint32_t message_type, response_type;
    void *replacement;
    memcpy(&message_type, message, sizeof(message_type));
    memcpy(&response_type, (unsigned char *)message + 0x18U, sizeof(response_type));
    if (message_type != 2U ||
        (response_type != ROTK_VOLUME_SPEAKER_AUX && response_type != ROTK_VOLUME_MIC_AUX)) return;
    memcpy(&replacement, (unsigned char *)message + 0x30U, sizeof(replacement));
    for (size_t index = 0U; index < ROTK_VOLUME_PENDING_LIMIT; ++index) {
        rotk_volume_pending *entry = &table->entries[index];
        if (entry->replacement == replacement && replacement != NULL) {
            memcpy((unsigned char *)message + 0x18U, &entry->legacy_type, sizeof(entry->legacy_type));
            memcpy((unsigned char *)message + 0x30U, &entry->original, sizeof(entry->original));
            (void)destroy(replacement);
            memset(entry, 0, sizeof(*entry));
            return;
        }
    }
}

/* The SDK owns pending replacements; the proxy still owns their originals. */
static void rotk_volume_clear(rotk_volume_table *table, rotk_volume_destroy_fn destroy) {
    for (size_t index = 0U; index < ROTK_VOLUME_PENDING_LIMIT; ++index) {
        rotk_volume_pending *entry = &table->entries[index];
        if (entry->original != NULL) (void)destroy(entry->original);
        memset(entry, 0, sizeof(*entry));
    }
}
#endif
