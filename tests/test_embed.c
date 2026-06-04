/*
 * test_embed.c — Test embedded/microcontroller scenarios.
 * Verifies zero dynamic allocation and small footprint.
 */
#define COMPILED_POLICY_IMPLEMENTATION
#include "../include/compiled_policy.h"
#include "../examples/ttt_policy.h"
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
    printf("=== Embedded Scenario Tests ===\n\n");

    /* Test 1: Static allocation only — hash table on stack */
    {
        int buckets[256];
        memset(buckets, -1, sizeof(buckets));
        PolicyHashTable ht = { .buckets = buckets, .nbuckets = 256 };
        policy_build_hash_table(&ht, &ttt_policy);

        char hash[17];
        policy_hash_state("         ", hash);
        int action = policy_lookup_hashed(&ht, &ttt_policy, hash);
        ASSERT(action >= 0, "Stack-allocated hash table works");
        printf("Stack HT: %d entries, lookup returned %d\n", ttt_policy.count, action);
    }

    /* Test 2: All policy data is const/static */
    {
        ASSERT(ttt_policy.entries != NULL, "Policy entries accessible");
        ASSERT(ttt_policy.count > 0, "Policy has entries");
        /* Verify no mutation happens */
        int orig_count = ttt_policy.count;
        char hash[17];
        policy_hash_state("         ", hash);
        policy_lookup(&ttt_policy, hash);
        ASSERT(ttt_policy.count == orig_count, "Policy unchanged after lookup");
    }

    /* Test 3: Small buffer sizes (typical MCU constraints) */
    {
        /* Hash table with minimal buckets */
        int small_buckets[64];
        memset(small_buckets, -1, sizeof(small_buckets));
        PolicyHashTable small_ht = { .buckets = small_buckets, .nbuckets = 64 };
        policy_build_hash_table(&small_ht, &ttt_policy);

        int found = 0;
        for (int i = 0; i < ttt_policy.count; i++) {
            int a = policy_lookup_hashed(&small_ht, &ttt_policy,
                                         ttt_policy.entries[i].state_hash);
            if (a >= 0) found++;
        }
        printf("Small HT (64 buckets): found %d/%d entries\n", found, ttt_policy.count);
        ASSERT(found == ttt_policy.count, "All entries found with small hash table");
    }

    /* Test 4: sizeof sanity checks */
    {
        printf("sizeof(PolicyEntry) = %zu bytes\n", sizeof(PolicyEntry));
        printf("sizeof(CompiledPolicy) = %zu bytes\n", sizeof(CompiledPolicy));
        printf("sizeof(PolicyHashTable) = %zu bytes\n", sizeof(PolicyHashTable));
        printf("Total policy data: %zu bytes\n",
               ttt_policy.count * sizeof(PolicyEntry));
        ASSERT(sizeof(PolicyEntry) <= 32, "PolicyEntry <= 32 bytes");
        ASSERT(ttt_policy.count * sizeof(PolicyEntry) < 4096,
               "Total policy < 4KB (fits MCU RAM)");
    }

    /* Test 5: Multiple lookups without re-initialization */
    {
        int buckets[128];
        memset(buckets, -1, sizeof(buckets));
        PolicyHashTable ht = { .buckets = buckets, .nbuckets = 128 };
        policy_build_hash_table(&ht, &ttt_policy);

        int ok = 1;
        for (int round = 0; round < 100 && ok; round++) {
            for (int i = 0; i < ttt_policy.count; i++) {
                int a = policy_lookup_hashed(&ht, &ttt_policy,
                                             ttt_policy.entries[i].state_hash);
                if (a < 0) { ok = 0; break; }
            }
        }
        ASSERT(ok, "100 rounds of all lookups succeed");
    }

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
