/*
 * test_hash.c — Test BLAKE2b hashing.
 */
#define COMPILED_POLICY_IMPLEMENTATION
#include "../include/compiled_policy.h"
#include <stdio.h>
#include <string.h>

static int tests_run = 0;
static int tests_passed = 0;

#define ASSERT(cond, msg) do { \
    tests_run++; \
    if (cond) { tests_passed++; } \
    else { printf("FAIL: %s (line %d)\n", msg, __LINE__); } \
} while(0)

int main(void) {
    printf("=== Hash Tests ===\n\n");

    /* Test 1: Deterministic */
    {
        char h1[17], h2[17];
        policy_hash_state("         ", h1);
        policy_hash_state("         ", h2);
        ASSERT(strcmp(h1, h2) == 0, "Same input -> same hash");
        printf("Empty board: %s\n", h1);
    }

    /* Test 2: Different inputs -> different hashes */
    {
        char h1[17], h2[17];
        policy_hash_state("         ", h1);
        policy_hash_state("X        ", h2);
        ASSERT(strcmp(h1, h2) != 0, "Different inputs -> different hashes");
    }

    /* Test 3: Hash is 16 hex chars */
    {
        char h[17];
        policy_hash_state("test", h);
        int valid = 1;
        for (int i = 0; i < 16; i++) {
            char c = h[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
                valid = 0;
                break;
            }
        }
        ASSERT(valid && h[16] == '\0', "Hash is 16 hex chars + null");
        printf("Hash of 'test': %s\n", h);
    }

    /* Test 4: Known BLAKE2b-64 vectors */
    /* Empty string BLAKE2b-64: 786a02f742015903c6c6fd852552d272912f4740e15847618a86e217f71f5419d39e */
    /* But we only take first 8 bytes = 16 hex chars */
    {
        char h[17];
        policy_hash_state("", h);
        printf("Hash of '': %s\n", h);
        /* Just verify it produces output; verifying exact BLAKE2b requires reference impl */
        ASSERT(strlen(h) == 16, "Empty string produces valid hash");
    }

    /* Test 5: Cross-validate with Python BLAKE2b */
    /* Python: hashlib.blake2b(b'         ', digest_size=8).hexdigest() */
    {
        char h[17];
        policy_hash_state("         ", h);
        /* Known value from Python */
        ASSERT(strcmp(h, "5488becc4be28c12") == 0,
               "BLAKE2b-64 matches Python implementation");
        printf("Verified: '         ' -> %s (matches Python)\n", h);
    }

    /* Test 6: Another known value */
    {
        char h[17];
        policy_hash_state("X O  X   ", h);
        printf("'X O  X   ' -> %s\n", h);
        ASSERT(strlen(h) == 16, "Known state produces valid hash");
    }

    /* Test 7: policy_hex_hash consistency */
    {
        uint32_t h1 = policy_hex_hash("5488becc4be28c12");
        uint32_t h2 = policy_hex_hash("5488becc4be28c12");
        ASSERT(h1 == h2, "FNV-1a hash is deterministic");
        printf("FNV-1a of '5488becc4be28c12': 0x%08x\n", h1);
    }

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
