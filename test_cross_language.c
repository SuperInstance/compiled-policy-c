/**
 * test_cross_language.c — Cross-language validation against reference vectors.
 *
 * Validates that the C BLAKE2b-64 implementation matches Python hashlib.
 * Test vectors derived from superinstance-ecosystem/test-vectors-blake2b.json
 * with BLAKE2b-64 (digest_size=8) expected values computed via Python hashlib.
 */

#include <stdio.h>
#include <string.h>
#include "compiled_policy.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  TEST: %s ... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

typedef struct {
    int         id;
    const char* state;
    const char* expected_blake2b_64;  /* 16 hex chars, digest_size=8 */
    const char* expected_blake2b_128; /* 32 hex chars, digest_size=16 */
} TestVector;

/* Vectors from test-vectors-blake2b.json, BLAKE2b-64 computed via Python */
static const TestVector vectors[] = {
    { 0, ".........",          "8bd4394b090b8869", "89fa3a927e254bea7218c405823999aa" },
    { 1, "X........",          "5ba5f669c7ee003b", "bd5885cc73b761cec505154b238f6234" },
    { 2, "XO.X.O.XO",          "16227f84280904d6", "33ecca5033d17fba55745ae13d73b461" },
    { 3, "XXX......",          "4ab8f912881aba43", "66c58ca508531138acbeee77d7eba49c" },
    { 4, "OOO......",          "58335f89c6a83941", "2cd3b5ad916491d7077c4582e15bf053" },
    { 5, "X.OX.OX.O",          "091c4f2e07cf6788", "2a0db01e3e1939602d78fe2ed0d4302d" },
    { 6, "XOXOXOXOX",          "65913381c0a15dac", "97e3dd13e4a3b0e400eb37936fa6a27b" },
    { 7, "",                   "e4a6a0577479b2b4", "cae66941d9efbd404e4d88758ea67670" },
    { 8, "a",                  "40f89e395b66422f", "27c35e6e9373877f29e562464e46497e" },
    { 9, " negotiation:accept ", "f4b6b1edd38f3ee2", "c66444be4724b637789f86b75272b180" },
};
#define N_VECTORS (sizeof(vectors) / sizeof(vectors[0]))

int main(void) {
    printf("=== Cross-Language Validation (C vs Python hashlib) ===\n");
    printf("Algorithm: BLAKE2b-64 (digest_size=8, 16 hex chars)\n");
    printf("Reference: Python hashlib.blake2b(digest_size=8)\n\n");

    /* Test 1: All vectors match expected BLAKE2b-64 */
    {
        int all_ok = 1;
        for (size_t i = 0; i < N_VECTORS; i++) {
            char got[17];
            policy_hash(vectors[i].state, got);
            if (memcmp(got, vectors[i].expected_blake2b_64, 16) != 0) {
                if (all_ok) printf("\n");
                printf("    FAIL vector %d: state=\"%s\"\n", vectors[i].id, vectors[i].state);
                printf("      expected: %.16s\n", vectors[i].expected_blake2b_64);
                printf("      got:      %.16s\n", got);
                all_ok = 0;
            }
        }
        TEST("all 10 vectors match Python BLAKE2b-64");
        if (all_ok) PASS();
        else FAIL("see above");
    }

    /* Test 2: Determinism — hash each vector twice, must match */
    {
        int ok = 1;
        for (size_t i = 0; i < N_VECTORS; i++) {
            char h1[17], h2[17];
            policy_hash(vectors[i].state, h1);
            policy_hash(vectors[i].state, h2);
            if (memcmp(h1, h2, 16) != 0) {
                ok = 0;
                printf("    vector %d: non-deterministic!\n", vectors[i].id);
            }
        }
        TEST("deterministic across all vectors");
        if (ok) PASS();
        else FAIL("non-deterministic output detected");
    }

    /* Test 3: No two different states produce the same hash */
    {
        int collision = 0;
        for (size_t i = 0; i < N_VECTORS && !collision; i++) {
            for (size_t j = i + 1; j < N_VECTORS && !collision; j++) {
                char h1[17], h2[17];
                policy_hash(vectors[i].state, h1);
                policy_hash(vectors[j].state, h2);
                if (memcmp(h1, h2, 16) == 0) {
                    collision = 1;
                    printf("    collision: vec %d & %d\n", vectors[i].id, vectors[j].id);
                }
            }
        }
        TEST("no collisions among test vectors");
        if (!collision) PASS();
        else FAIL("hash collision detected");
    }

    /* Test 4: Output format is valid lowercase hex */
    {
        int ok = 1;
        for (size_t i = 0; i < N_VECTORS; i++) {
            char h[17];
            policy_hash(vectors[i].state, h);
            for (int j = 0; j < 16; j++) {
                char c = h[j];
                if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
                    ok = 0;
                    printf("    vector %d: invalid hex char '%c' at pos %d\n", vectors[i].id, c, j);
                }
            }
            if (h[16] != '\0') {
                ok = 0;
                printf("    vector %d: not NUL-terminated\n", vectors[i].id);
            }
        }
        TEST("output is valid lowercase hex (16 chars + NUL)");
        if (ok) PASS();
        else FAIL("invalid format");
    }

    /* Test 5: BLAKE2b-64 ≠ first 16 chars of BLAKE2b-128 */
    {
        int diff_count = 0;
        for (size_t i = 0; i < N_VECTORS; i++) {
            char h[17];
            policy_hash(vectors[i].state, h);
            if (memcmp(h, vectors[i].expected_blake2b_128, 16) != 0) {
                diff_count++;
            }
        }
        TEST("BLAKE2b-64 differs from BLAKE2b-128 prefix (different algorithms)");
        /* All should differ since digest_size affects the hash */
        if (diff_count == (int)N_VECTORS) PASS();
        else FAIL("some vectors match BLAKE2b-128 prefix unexpectedly");
    }

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
