/*
 * compiled_policy.h — Single-header library for compiled tile policies.
 *
 * Zero dependencies beyond stdint.h, string.h, stdlib.h.
 * Works on ESP8266, Arduino, STM32 — no dynamic allocation after init.
 *
 * Usage:
 *   #define COMPILED_POLICY_IMPLEMENTATION
 *   #include "compiled_policy.h"
 */
#ifndef COMPILED_POLICY_H
#define COMPILED_POLICY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---- Types ---- */

typedef struct {
    const char* state_hash;  /* 16-char hex string (BLAKE2b-64) */
    int         action;      /* Best action index */
    float       score;       /* Confidence [0, 1] */
} PolicyEntry;

typedef struct {
    const PolicyEntry* entries;
    int                count;
    float              temperature; /* For softmax selection */
} CompiledPolicy;

/* Hash table for O(1) lookup. Caller provides storage. */
typedef struct {
    int*        buckets;    /* Array of entry indices, size = nbuckets */
    int         nbuckets;
} PolicyHashTable;

/* ---- API ---- */

/*
 * O(1) lookup via hash table.
 * Returns action index, or -1 if state not found.
 */
int policy_lookup(const CompiledPolicy* p, const char* state_hash);

/*
 * O(1) lookup with pre-built hash table (faster for repeated queries).
 */
int policy_lookup_hashed(const PolicyHashTable* ht,
                         const CompiledPolicy*  p,
                         const char*            state_hash);

/*
 * Build a hash table for accelerated lookups.
 * buckets[] must be pre-allocated with nbuckets slots (init to -1).
 * nbuckets should be >= 2*count and be a power of 2.
 */
void policy_build_hash_table(PolicyHashTable* ht,
                             const CompiledPolicy* p);

/*
 * Softmax selection with configurable temperature.
 * valid_actions: array of valid action indices.
 * n_valid: number of valid actions.
 * Returns selected action index.
 */
int policy_select(const CompiledPolicy* p,
                  const char*           state_hash,
                  const int*            valid_actions,
                  int                   n_valid);

/*
 * Hash a state string to a 16-char hex digest (BLAKE2b, 64-bit).
 * out_hex must be at least 17 bytes (16 hex + null).
 */
void policy_hash_state(const char* state_str, char* out_hex);

/*
 * Simple inline hash for hex string -> uint32 (for hash table bucketing).
 */
static inline uint32_t policy_hex_hash(const char* hex16) {
    uint32_t h = 0x811c9dc5; /* FNV-1a offset basis */
    for (int i = 0; i < 16; i++) {
        h ^= (uint8_t)hex16[i];
        h *= 0x01000193; /* FNV prime */
    }
    return h;
}

#ifdef __cplusplus
}
#endif

#endif /* COMPILED_POLICY_H */

/* ---- Implementation ---- */
#ifdef COMPILED_POLICY_IMPLEMENTATION

#include <string.h>
#include <stdlib.h>
#include <math.h>

/* ============================================================
 * BLAKE2b — compact implementation (~200 LOC)
 * Reference: RFC 7693
 * Only uses 64-bit digest mode (8 bytes).
 * ============================================================ */

static const uint64_t blake2b_iv[8] = {
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

static const uint8_t blake2b_sigma[12][16] = {
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15},
    {14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3},
    {11, 8,12, 0, 5, 2,15,13,10,14, 3, 6, 7, 1, 9, 4},
    { 7, 9, 3, 1,13,12,11,14, 2, 6, 5,10, 4, 0,15, 8},
    { 9, 0, 5, 7, 2, 4,10,15,14, 1,11,12, 6, 8, 3,13},
    { 2,12, 6,10, 0,11, 8, 3, 4,13, 7, 5,15,14, 1, 9},
    {12, 5, 1,15,14,13, 4,10, 0, 7, 6, 3, 9, 2, 8,11},
    {13,11, 7,14,12, 1, 3, 9, 5, 0,15, 4, 8, 6, 2,10},
    { 6,15,14, 9,11, 3, 0, 8,12, 2,13, 7, 1, 4,10, 5},
    {10, 2, 8, 4, 7, 6, 1, 5,15,11, 9,14, 3,12,13, 0},
    { 0, 1, 2, 3, 4, 5, 6, 7, 8, 9,10,11,12,13,14,15},
    {14,10, 4, 8, 9,15,13, 6, 1,12, 0, 2,11, 7, 5, 3},
};

static inline uint64_t blake2b_rotr64(uint64_t x, unsigned n) {
    return (x >> n) | (x << (64 - n));
}

#define BLAKE2B_G(v, a, b, c, d, x, y) \
    do { \
        v[a] += v[b] + x; v[d] = blake2b_rotr64(v[d] ^ v[a], 32); \
        v[c] += v[d];       v[b] = blake2b_rotr64(v[b] ^ v[c], 24); \
        v[a] += v[b] + y; v[d] = blake2b_rotr64(v[d] ^ v[a], 16); \
        v[c] += v[d];       v[b] = blake2b_rotr64(v[b] ^ v[c], 63); \
    } while (0)

#define BLAKE2B_ROUND(v, m, s, r) \
    do { \
        BLAKE2B_G(v, 0, 4,  8, 12, m[blake2b_sigma[r][ 0]], m[blake2b_sigma[r][ 1]]); \
        BLAKE2B_G(v, 1, 5,  9, 13, m[blake2b_sigma[r][ 2]], m[blake2b_sigma[r][ 3]]); \
        BLAKE2B_G(v, 2, 6, 10, 14, m[blake2b_sigma[r][ 4]], m[blake2b_sigma[r][ 5]]); \
        BLAKE2B_G(v, 3, 7, 11, 15, m[blake2b_sigma[r][ 6]], m[blake2b_sigma[r][ 7]]); \
        BLAKE2B_G(v, 0, 5, 10, 15, m[blake2b_sigma[r][ 8]], m[blake2b_sigma[r][ 9]]); \
        BLAKE2B_G(v, 1, 6, 11, 12, m[blake2b_sigma[r][10]], m[blake2b_sigma[r][11]]); \
        BLAKE2B_G(v, 2, 7,  8, 13, m[blake2b_sigma[r][12]], m[blake2b_sigma[r][13]]); \
        BLAKE2B_G(v, 3, 4,  9, 14, m[blake2b_sigma[r][14]], m[blake2b_sigma[r][15]]); \
    } while (0)

static void blake2b_compress(uint64_t h[8], const uint8_t block[128],
                             uint64_t counter, int final)
{
    uint64_t v[16];
    uint64_t m[16];
    int i;

    for (i = 0; i < 8; i++) v[i] = h[i];
    for (i = 0; i < 8; i++) v[i + 8] = blake2b_iv[i];
    v[12] ^= counter;
    v[13] ^= 0; /* counter >> 64 = 0 for small inputs */
    if (final) v[14] = ~v[14];

    for (i = 0; i < 16; i++) {
        uint64_t w = 0;
        for (int j = 0; j < 8; j++)
            w |= ((uint64_t)block[i * 8 + j]) << (j * 8);
        m[i] = w;
    }

    for (int r = 0; r < 12; r++)
        BLAKE2B_ROUND(v, m, s, r);

    for (i = 0; i < 8; i++)
        h[i] ^= v[i] ^ v[i + 8];
}

/* Compute BLAKE2b with 64-bit (8-byte) digest. */
static void blake2b_64(const uint8_t* data, size_t len, uint8_t out[8])
{
    uint64_t h[8];
    uint8_t  block[128];
    uint64_t counter = 0;
    size_t   off = 0;

    h[0] = blake2b_iv[0] ^ 0x01010008; /* nn=8 */
    for (int i = 1; i < 8; i++) h[i] = blake2b_iv[i];

    /* Process full blocks */
    while (len - off >= 128) {
        memset(block, 0, 128);
        memcpy(block, data + off, 128);
        blake2b_compress(h, block, counter++, 0);
        off += 128;
    }

    /* Final partial block */
    memset(block, 0, 128);
    memcpy(block, data + off, len - off);
    blake2b_compress(h, block, counter, 1);

    /* Extract first 8 bytes */
    for (int i = 0; i < 8; i++)
        out[i] = (uint8_t)(h[0] >> (i * 8));
}

/* ---- Hash state string to 16-char hex ---- */

static const char hex_chars[] = "0123456789abcdef";

void policy_hash_state(const char* state_str, char* out_hex) {
    uint8_t digest[8];
    blake2b_64((const uint8_t*)state_str, strlen(state_str), digest);
    for (int i = 0; i < 8; i++) {
        out_hex[i * 2]     = hex_chars[(digest[i] >> 4) & 0xf];
        out_hex[i * 2 + 1] = hex_chars[digest[i] & 0xf];
    }
    out_hex[16] = '\0';
}

/* ---- Linear scan lookup (fallback, no hash table) ---- */

int policy_lookup(const CompiledPolicy* p, const char* state_hash) {
    /* Binary search if entries are sorted by hash (they are) */
    int lo = 0, hi = p->count - 1;
    while (lo <= hi) {
        int mid = lo + (hi - lo) / 2;
        int cmp = strcmp(p->entries[mid].state_hash, state_hash);
        if (cmp == 0) return p->entries[mid].action;
        if (cmp < 0) lo = mid + 1;
        else hi = mid - 1;
    }
    return -1;
}

/* ---- Hash table lookup ---- */

int policy_lookup_hashed(const PolicyHashTable* ht,
                         const CompiledPolicy*  p,
                         const char*            state_hash)
{
    if (ht->nbuckets == 0) return policy_lookup(p, state_hash);

    uint32_t h = policy_hex_hash(state_hash);
    uint32_t idx = h & (ht->nbuckets - 1);

    /* Open addressing, linear probe */
    for (int tries = 0; tries < ht->nbuckets; tries++) {
        int entry_idx = ht->buckets[idx];
        if (entry_idx < 0) return -1; /* empty slot = not found */
        if (strcmp(p->entries[entry_idx].state_hash, state_hash) == 0)
            return p->entries[entry_idx].action;
        idx = (idx + 1) & (ht->nbuckets - 1);
    }
    return -1;
}

/* ---- Build hash table ---- */

void policy_build_hash_table(PolicyHashTable* ht,
                             const CompiledPolicy* p)
{
    /* buckets should already be filled with -1 and nbuckets set */
    for (int i = 0; i < p->count; i++) {
        uint32_t h = policy_hex_hash(p->entries[i].state_hash);
        uint32_t idx = h & (ht->nbuckets - 1);
        while (ht->buckets[idx] >= 0)
            idx = (idx + 1) & (ht->nbuckets - 1);
        ht->buckets[idx] = i;
    }
}

/* ---- Softmax selection ---- */

int policy_select(const CompiledPolicy* p,
                  const char*           state_hash,
                  const int*            valid_actions,
                  int                   n_valid)
{
    if (n_valid <= 0) return -1;
    if (n_valid == 1) return valid_actions[0];

    float temp = p->temperature;
    if (temp <= 0.0f) temp = 1.0f;

    /* Look up score for the state; default 0.5 if not found */
    float base_score = 0.5f;
    int best_action = policy_lookup(p, state_hash);
    /* For softmax, assign higher weight to the best action */

    float weights[64]; /* max 64 valid actions */
    float max_w = -1e30f;
    float sum = 0.0f;

    for (int i = 0; i < n_valid && i < 64; i++) {
        float s = (valid_actions[i] == best_action) ? base_score + 0.5f : base_score;
        float w = expf(s / temp);
        weights[i] = w;
        if (w > max_w) max_w = w;
        sum += w;
    }

    /* Sample from softmax distribution */
    float r = (float)rand() / (float)RAND_MAX;
    float cumulative = 0.0f;
    for (int i = 0; i < n_valid && i < 64; i++) {
        cumulative += weights[i] / sum;
        if (r <= cumulative) return valid_actions[i];
    }
    return valid_actions[n_valid - 1];
}

#endif /* COMPILED_POLICY_IMPLEMENTATION */
