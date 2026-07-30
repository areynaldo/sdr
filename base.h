#ifndef BASE_H
#define BASE_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include <math.h>
#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

#if defined(_MSC_VER)
#define DEBUG_BREAK() __debugbreak()
#else
#define DEBUG_BREAK() __builtin_trap()
#endif

#define CLAMP(x, a, b) ((x) < (a) ? (a) : ((x) > (b) ? (b) : (x)))

#define ASSERT(expr) do { \
    if (!(expr)) { \
        fprintf(stderr, "ASSERT_BREAK failed: %s (%s:%d)\n", #expr, __FILE__, __LINE__); \
        DEBUG_BREAK(); \
    } \
} while (0)

typedef float float32_t;
typedef double float64_t;


#define KiB(x) ((x)*1024LL)
#define MiB(x) (KiB(x))*(1024LL)
#define GiB(x) (MiB(x))*(1024LL)

#define DEFAULT_ALIGN 64

static inline bool size_is_power_of_2(size_t x) {
    return x != 0 && (x & (x - 1)) == 0;
}

typedef struct arena_t arena_t;
struct arena_t {
    uint8_t *buffer;
    size_t capacity;
    size_t offset;
};

static inline bool arena_init(arena_t *arena, size_t capacity) {
    ASSERT(arena);

    arena->buffer = NULL;
    arena->capacity = 0;
    arena->offset = 0;

    if (capacity == 0) {
        return false;
    }

    arena->buffer = (uint8_t *)malloc(capacity);
    if (!arena->buffer) {
        return false;
    }

    arena->capacity = capacity;
    return true;
}

static inline void *arena_alloc_align(arena_t *arena, size_t size, size_t align) {
    ASSERT(arena);
    ASSERT(size_is_power_of_2(align));

    if (!arena->buffer || size == 0) {
        return NULL;
    }

    // Round the current offset up to the requested alignment.
    size_t mask = align - 1;
    size_t aligned_offset = (arena->offset + mask) & ~mask;
    if (aligned_offset > arena->capacity || size > arena->capacity - aligned_offset) {
        return NULL;
    }

    void *pointer = arena->buffer + aligned_offset;
    arena->offset = aligned_offset + size;
    return pointer;
}

static inline void *arena_alloc(arena_t *arena, size_t size) {
    ASSERT(arena);

    void *pointer = arena_alloc_align(arena, size, DEFAULT_ALIGN);
    return pointer;
}

static inline void arena_reset(arena_t *arena) {
    ASSERT(arena);

    arena->offset = 0;
}

static inline void arena_release(arena_t *arena) {
    if (!arena) {
        return;
    }

    free(arena->buffer);
    arena->buffer = NULL;
    arena->capacity = 0;
    arena->offset = 0;
}

typedef struct string_t string_t;
struct string_t {
    char *str;
    size_t length;
};

typedef struct string_intern_table_t string_intern_table_t;
struct string_intern_table_t {
    uint64_t *hashes;
    size_t *offsets;
    size_t *lengths;
    size_t capacity;
    size_t count;
};

static const uint64_t STRING_HASH_UNUSED = 0xffffffffffffffffULL;

static inline uint64_t string_hash_bytes(const char *data, size_t length) {
    uint64_t hash = 1469598103934665603ULL;
    for (size_t i = 0; i < length; ++i) {
        hash ^= (uint8_t)data[i];
        hash *= 1099511628211ULL;
    }
    return hash;
}

static inline bool string_intern_table_init(string_intern_table_t *table, size_t capacity) {
    ASSERT(table);
    if (capacity < 8) {
        capacity = 8;
    }

    table->hashes = (uint64_t *)malloc(sizeof(uint64_t) * capacity);
    table->offsets = (size_t *)malloc(sizeof(size_t) * capacity);
    table->lengths = (size_t *)malloc(sizeof(size_t) * capacity);
    if (!table->hashes || !table->offsets || !table->lengths) {
        free(table->hashes);
        free(table->offsets);
        free(table->lengths);
        table->hashes = NULL;
        table->offsets = NULL;
        table->lengths = NULL;
        table->capacity = 0;
        table->count = 0;
        return false;
    }

    memset(table->hashes, 0xff, sizeof(uint64_t) * capacity);
    table->capacity = capacity;
    table->count = 0;
    return true;
}

static inline void string_intern_table_clear(string_intern_table_t *table) {
    ASSERT(table);
    if (!table->hashes) {
        return;
    }
    memset(table->hashes, 0xff, sizeof(uint64_t) * table->capacity);
    table->count = 0;
}

static inline void string_intern_table_release(string_intern_table_t *table) {
    if (!table) {
        return;
    }

    free(table->hashes);
    free(table->offsets);
    free(table->lengths);
    table->hashes = NULL;
    table->offsets = NULL;
    table->lengths = NULL;
    table->capacity = 0;
    table->count = 0;
}

static inline const char *string_intern(string_intern_table_t *table, arena_t *arena, const char *str, size_t length) {
    ASSERT(table);
    ASSERT(arena);
    ASSERT(str);
    ASSERT(table->capacity > 0);

    if (!table->hashes) {
        return NULL;
    }

    uint64_t hash = string_hash_bytes(str, length);
    size_t index = hash % table->capacity;
    while (table->hashes[index] != STRING_HASH_UNUSED) {
        if (table->hashes[index] == hash && table->lengths[index] == length) {
            const char *candidate = (const char *)(arena->buffer + table->offsets[index]);
            if (memcmp(candidate, str, length) == 0) {
                return candidate;
            }
        }
        index = (index + 1) % table->capacity;
    }

    char *copy = (char *)arena_alloc_align(arena, length + 1, 1);
    if (!copy) {
        return NULL;
    }
    memcpy(copy, str, length);
    copy[length] = '\0';

    table->hashes[index] = hash;
    table->offsets[index] = (size_t)(copy - (char *)arena->buffer);
    table->lengths[index] = length;
    table->count++;

    return copy;
}

static inline const char *string_intern_cstr(string_intern_table_t *table, arena_t *arena, const char *str) {
    return string_intern(table, arena, str, strlen(str));
}

char *read_file_to_string_zero_terminated(arena_t *arena, char *file_path) {
    ASSERT(arena);
    ASSERT(file_path);

    char *result;

    FILE *file = NULL;
    file = fopen(file_path, "rb");
    if (!file) {
        return NULL;
    }

    fseek(file, 0, SEEK_END);
    size_t length = ftell(file);
    fseek(file, 0, SEEK_SET);

    result = (char *)arena_alloc(arena, length);
    fread(result, 1, length, file);

    ASSERT(result);
    return result;
}

#endif // BASE_H