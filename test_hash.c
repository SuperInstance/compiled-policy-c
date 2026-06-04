/**
 * test_hash.c — Test BLAKE2b hashing consistency.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compiled_policy.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  TEST: %s ... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

int main(void) {
    printf("=== Hash Tests ===\n\n");

    /* Test 1: Deterministic */
    {
        TEST("hash is deterministic");
        char h1[17], h2[17];
        policy_hash("         ", h1);
        policy_hash("         ", h2);
        if (memcmp(h1, h2, 17) == 0) PASS();
        else FAIL("hashes differ");
    }

    /* Test 2: Different inputs → different hashes */
    {
        TEST("different states → different hashes");
        char h1[17], h2[17];
        policy_hash("         ", h1);
        policy_hash("X        ", h2);
        if (memcmp(h1, h2, 16) != 0) PASS();
        else FAIL("hashes collide");
    }

    /* Test 3: Output is valid hex */
    {
        TEST("output is valid hex");
        char h[17];
        policy_hash("test12345", h);
        int valid = 1;
        for (int i = 0; i < 16; i++) {
            char c = h[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
                valid = 0;
                break;
            }
        }
        if (valid && h[16] == '\0') PASS();
        else FAIL("invalid hex characters");
    }

    /* Test 4: Known hash value (verified against Python hashlib) */
    {
        TEST("known hash: empty TTT board");
        char h[17];
        policy_hash("         ", h);
        /* Python: hashlib.blake2b(b'         ', digest_size=8).hexdigest() */
        const char* expected = "5488becc4be28c12";
        if (memcmp(h, expected, 16) == 0) PASS();
        else { printf("got %.16s expected %.16s\n", h, expected); FAIL("hash mismatch"); }
    }

    /* Test 5: Known hash value for another state */
    {
        TEST("known hash: 'X O X O  '");
        char h[17];
        policy_hash("X O X O  ", h);
        const char* expected = "d595966958fae8a2";
        /* Verify with Python */
        if (memcmp(h, expected, 16) == 0) PASS();
        else { printf("got %.16s expected %.16s\n", h, expected); FAIL("hash mismatch"); }
    }

    /* Test 6: Empty string */
    {
        TEST("hash empty string");
        char h[17];
        policy_hash("", h);
        /* Should not crash and should be valid hex */
        int valid = 1;
        for (int i = 0; i < 16; i++) {
            char c = h[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) {
                valid = 0;
                break;
            }
        }
        if (valid) PASS();
        else FAIL("invalid output");
    }

    /* Test 7: Long string */
    {
        TEST("hash long string (42 chars)");
        char state[43];
        memset(state, ' ', 42);
        state[42] = '\0';
        state[3] = 'X';
        char h[17];
        policy_hash(state, h);
        int valid = 1;
        for (int i = 0; i < 16; i++) {
            char c = h[i];
            if (!((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'))) valid = 0;
        }
        if (valid) PASS();
        else FAIL("invalid output");
    }

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
