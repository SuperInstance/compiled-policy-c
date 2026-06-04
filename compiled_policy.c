/**
 * compiled_policy.c — Implementation of compiled tile policies.
 *
 * BLAKE2b-64 hash → 16-char hex key → hash table O(1) lookup.
 * Softmax action selection with configurable temperature.
 * Zero dependencies beyond string.h, stdint.h, stdlib.h.
 */

#include "compiled_policy.h"

#include <string.h>
#include <stdlib.h>

/* ================================================================
 *  BLAKE2b — Reference-based compact implementation
 * ================================================================ */

typedef struct {
    uint64_t h[8];
    uint64_t t[2];       /* byte counter */
    uint64_t f[2];       /* finalization flags */
    uint8_t  buf[128];
    size_t   buflen;
    size_t   outlen;
} blake2b_state;

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

static inline uint64_t rotr64(uint64_t x, int n) {
    return (x >> n) | (x << (64 - n));
}

static inline uint64_t load64_le(const void* src) {
    const uint8_t* p = (const uint8_t*)src;
    return (uint64_t)p[0]        | ((uint64_t)p[1] << 8)  |
           ((uint64_t)p[2] << 16)| ((uint64_t)p[3] << 24) |
           ((uint64_t)p[4] << 32)| ((uint64_t)p[5] << 40) |
           ((uint64_t)p[6] << 48)| ((uint64_t)p[7] << 56);
}

#define G(r,i,a,b,c,d) do {                              \
    a = a + b + m[blake2b_sigma[r][2*i+0]];              \
    d = rotr64(d ^ a, 32); c = c + d;                    \
    b = rotr64(b ^ c, 24);                               \
    a = a + b + m[blake2b_sigma[r][2*i+1]];              \
    d = rotr64(d ^ a, 16); c = c + d;                    \
    b = rotr64(b ^ c, 63);                               \
} while(0)

#define ROUND(r) do {                                     \
    G(r,0,v[0],v[4],v[ 8],v[12]);                        \
    G(r,1,v[1],v[5],v[ 9],v[13]);                        \
    G(r,2,v[2],v[6],v[10],v[14]);                        \
    G(r,3,v[3],v[7],v[11],v[15]);                        \
    G(r,4,v[0],v[5],v[10],v[15]);                        \
    G(r,5,v[1],v[6],v[11],v[12]);                        \
    G(r,6,v[2],v[7],v[ 8],v[13]);                        \
    G(r,7,v[3],v[4],v[ 9],v[14]);                        \
} while(0)

static void blake2b_compress(blake2b_state* S, const uint8_t block[128]) {
    uint64_t m[16];
    uint64_t v[16];
    int i;

    for (i = 0; i < 16; i++)
        m[i] = load64_le(block + i * 8);

    for (i = 0; i < 8; i++) {
        v[i] = S->h[i];
        v[i + 8] = blake2b_iv[i];
    }
    v[12] ^= S->t[0];
    v[13] ^= S->t[1];
    v[14] ^= S->f[0];
    v[15] ^= S->f[1];

    ROUND(0); ROUND(1); ROUND(2); ROUND(3);
    ROUND(4); ROUND(5); ROUND(6); ROUND(7);
    ROUND(8); ROUND(9); ROUND(10); ROUND(11);

    for (i = 0; i < 8; i++)
        S->h[i] ^= v[i] ^ v[i + 8];
}

#undef G
#undef ROUND

static void blake2b_increment_counter(blake2b_state* S, uint64_t inc) {
    S->t[0] += inc;
    if (S->t[0] < inc) S->t[1]++;
}

static void blake2b_init(blake2b_state* S, size_t outlen) {
    uint8_t param[64];
    memset(param, 0, 64);
    param[0] = (uint8_t)outlen;   /* digest length */
    param[2] = 1; /* fanout */
    param[3] = 1; /* depth */

    memcpy(S->h, blake2b_iv, 64);
    /* XOR parameter block into h[0..1] */
    for (int i = 0; i < 8; i++)
        S->h[i] ^= load64_le(param + i * 8);

    S->t[0] = 0; S->t[1] = 0;
    S->f[0] = 0; S->f[1] = 0;
    S->buflen = 0;
    S->outlen = outlen;
}

static void blake2b_update(blake2b_state* S, const uint8_t* data, size_t len) {
    while (len > 0) {
        size_t left = 128 - S->buflen;
        size_t fill = (len < left) ? len : left;
        memcpy(S->buf + S->buflen, data, fill);
        S->buflen += fill;
        data += fill;
        len -= fill;

        if (S->buflen == 128) {
            blake2b_increment_counter(S, 128);
            blake2b_compress(S, S->buf);
            S->buflen = 0;
        }
    }
}

static void blake2b_final(blake2b_state* S, uint8_t* out) {
    uint8_t padding = 128 - S->buflen;
    if (padding > 0)
        memset(S->buf + S->buflen, 0, padding);

    blake2b_increment_counter(S, (uint64_t)S->buflen);
    S->f[0] = (uint64_t)-1; /* final flag */
    blake2b_compress(S, S->buf);

    /* Output little-endian */
    for (size_t i = 0; i < S->outlen; i++) {
        out[i] = (uint8_t)(S->h[i >> 3] >> (8 * (i & 7)));
    }
}

static void blake2b_simple(const uint8_t* data, size_t data_len,
                           uint8_t* out, size_t outlen) {
    blake2b_state S;
    blake2b_init(&S, outlen);
    blake2b_update(&S, data, data_len);
    blake2b_final(&S, out);
}

/* ================================================================
 *  Internal hash table
 * ================================================================ */

#define CP_HASH_BUCKETS 256

typedef struct {
    const PolicyEntry* entry;
    uint32_t           hash_val;
} _BucketNode;

typedef struct {
    _BucketNode* nodes;
    int          count;
    int          capacity;
} _Bucket;

typedef struct {
    _Bucket buckets[CP_HASH_BUCKETS];
    int     initialized;
} _HashTable;

static _HashTable _ht;

static uint32_t _hash_hex16(const char* hex16) {
    /* FNV-1a on the 16 hex chars */
    uint32_t h = 2166136261u;
    for (int i = 0; i < 16; i++) {
        h ^= (uint8_t)hex16[i];
        h *= 16777619u;
    }
    return h;
}

static void _ht_insert(const PolicyEntry* e) {
    uint32_t hv = _hash_hex16(e->hash);
    int bi = hv % CP_HASH_BUCKETS;
    _Bucket* b = &_ht.buckets[bi];
    if (b->count >= b->capacity) {
        int new_cap = b->capacity == 0 ? 4 : b->capacity * 2;
        _BucketNode* new_nodes = (_BucketNode*)realloc(b->nodes, new_cap * sizeof(_BucketNode));
        if (!new_nodes) return;
        b->nodes = new_nodes;
        b->capacity = new_cap;
    }
    b->nodes[b->count].entry = e;
    b->nodes[b->count].hash_val = hv;
    b->count++;
}

static const PolicyEntry* _ht_find(const char* hash) {
    uint32_t hv = _hash_hex16(hash);
    int bi = hv % CP_HASH_BUCKETS;
    _Bucket* b = &_ht.buckets[bi];
    for (int i = 0; i < b->count; i++) {
        if (b->nodes[i].hash_val == hv && memcmp(b->nodes[i].entry->hash, hash, 16) == 0) {
            return b->nodes[i].entry;
        }
    }
    return NULL;
}

static void _ht_build(const CompiledPolicy* p) {
    static const CompiledPolicy* last_p = NULL;
    if (last_p == p && _ht.initialized) return;
    /* Free old */
    for (int i = 0; i < CP_HASH_BUCKETS; i++) {
        free(_ht.buckets[i].nodes);
        _ht.buckets[i].nodes = NULL;
        _ht.buckets[i].count = 0;
        _ht.buckets[i].capacity = 0;
    }
    for (int i = 0; i < p->count; i++) {
        _ht_insert(&p->entries[i]);
    }
    _ht.initialized = 1;
    last_p = p;
}

/* ================================================================
 *  Softmax helper — expf approximation (avoids libm on MCU)
 * ================================================================ */

static float expf_approx(float x) {
    /* Schraudolph 1999: "A Fast, Compact Approximation of the Exponential Function" */
    if (x > 88.0f) x = 88.0f;
    if (x < -88.0f) x = -88.0f;
    union { float f; int32_t i; } u;
    u.i = (int32_t)(12102203.0f * x + 1064866805.0f);
    return u.f;
}

/* ================================================================
 *  Public API
 * ================================================================ */

static const char _version[] = "1.0.0";

const char* policy_version(void) {
    return _version;
}

void policy_hash(const char* state, char* out_hex16) {
    uint8_t digest[8];
    size_t len = strlen(state);
    blake2b_simple((const uint8_t*)state, len, digest, 8);
    /* Encode as 16 hex chars */
    static const char hex[] = "0123456789abcdef";
    for (int i = 0; i < 8; i++) {
        out_hex16[i*2+0] = hex[digest[i] >> 4];
        out_hex16[i*2+1] = hex[digest[i] & 0x0f];
    }
    out_hex16[16] = '\0';
}

int policy_lookup(const CompiledPolicy* p, const char* hash) {
    _ht_build(p);
    const PolicyEntry* e = _ht_find(hash);
    return e ? e->action : -1;
}

int policy_select(const CompiledPolicy* p, const char* hash,
                  const int* valid, int n_valid)
{
    if (!valid || n_valid <= 0) return -1;

    _ht_build(p);
    const PolicyEntry* e = _ht_find(hash);

    if (!e || p->temperature <= 0.0f) {
        /* Greedy: return looked-up action if valid, else first valid */
        if (e) {
            for (int i = 0; i < n_valid; i++) {
                if (valid[i] == e->action) return e->action;
            }
        }
        return valid[0];
    }

    /* Softmax with temperature over valid actions */
    float scores[64];
    if (n_valid > 64) n_valid = 64;

    float base_score = e ? e->score : 0.0f;
    for (int i = 0; i < n_valid; i++) {
        if (e && valid[i] == e->action) {
            scores[i] = base_score;
        } else {
            scores[i] = base_score * 0.1f;
        }
    }

    /* Softmax */
    float max_s = scores[0];
    for (int i = 1; i < n_valid; i++) {
        if (scores[i] > max_s) max_s = scores[i];
    }

    float exps[64];
    float sum = 0.0f;
    float inv_temp = 1.0f / p->temperature;
    for (int i = 0; i < n_valid; i++) {
        exps[i] = expf_approx((scores[i] - max_s) * inv_temp);
        sum += exps[i];
    }

    /* Weighted random selection using LCG */
    static uint32_t _rng_state = 0;
    if (_rng_state == 0) _rng_state = 12345;
    _rng_state = _rng_state * 1103515245u + 12345u;
    float r = (float)(_rng_state & 0x7FFFFFFF) / (float)0x7FFFFFFF;
    r *= sum;

    float acc = 0.0f;
    for (int i = 0; i < n_valid; i++) {
        acc += exps[i];
        if (r <= acc) return valid[i];
    }
    return valid[n_valid - 1];
}
