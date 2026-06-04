/*
 * test_lookup.c — Test policy lookup correctness.
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
    printf("=== Lookup Tests ===\n\n");

    /* Build hash table */
    int buckets[256];
    memset(buckets, -1, sizeof(buckets));
    PolicyHashTable ht = { .buckets = buckets, .nbuckets = 256 };
    policy_build_hash_table(&ht, &ttt_policy);

    /* Test 1: Empty board should be found */
    {
        char hash[17];
        policy_hash_state("         ", hash);
        int action = policy_lookup(&ttt_policy, hash);
        printf("Empty board hash: %s -> action %d\n", hash, action);
        ASSERT(action >= 0, "Empty board found via binary search");

        int action2 = policy_lookup_hashed(&ht, &ttt_policy, hash);
        ASSERT(action2 == action, "Hash table lookup matches binary search");
    }

    /* Test 2: Consistency between binary search and hash table */
    {
        int mismatches = 0;
        for (int i = 0; i < ttt_policy.count; i++) {
            int a1 = policy_lookup(&ttt_policy, ttt_policy.entries[i].state_hash);
            int a2 = policy_lookup_hashed(&ht, &ttt_policy, ttt_policy.entries[i].state_hash);
            if (a1 != a2 || a1 != ttt_policy.entries[i].action) mismatches++;
        }
        ASSERT(mismatches == 0, "All entries consistent across lookup methods");
        printf("Verified %d entries consistent\n", ttt_policy.count);
    }

    /* Test 3: Unknown state returns -1 */
    {
        int action = policy_lookup(&ttt_policy, "0000000000000000");
        ASSERT(action == -1, "Unknown state returns -1");
        action = policy_lookup_hashed(&ht, &ttt_policy, "0000000000000000");
        ASSERT(action == -1, "Unknown state returns -1 (hash table)");
    }

    /* Test 4: Every entry is retrievable */
    {
        int all_found = 1;
        for (int i = 0; i < ttt_policy.count; i++) {
            int a = policy_lookup(&ttt_policy, ttt_policy.entries[i].state_hash);
            if (a < 0) { all_found = 0; break; }
        }
        ASSERT(all_found, "All entries retrievable via binary search");
    }

    printf("\n%d/%d tests passed\n", tests_passed, tests_run);
    return (tests_passed == tests_run) ? 0 : 1;
}
