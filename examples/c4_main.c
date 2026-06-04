/*
 * c4_main.c — Connect4 example using compiled policy.
 */
#define COMPILED_POLICY_IMPLEMENTATION
#include "../include/compiled_policy.h"
#include "c4_policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("=== Connect4 Compiled Policy Demo ===\n\n");

    /* Build hash table */
    int buckets[64];
    memset(buckets, -1, sizeof(buckets));
    PolicyHashTable ht = { .buckets = buckets, .nbuckets = 64 };
    policy_build_hash_table(&ht, &c4_policy);

    /* Demo: look up a few states */
    const char* test_hashes[] = {
        "a1b2c3d4e5f6a7b8",
        "3e4f5a6b7c1d2e3f4",
        "5d6e7f1a2b3c4d5e6f7",
        "nonexistent000000",
    };

    for (int i = 0; i < 4; i++) {
        int action = policy_lookup_hashed(&ht, &c4_policy, test_hashes[i]);
        printf("State %s -> action %d\n", test_hashes[i], action);
    }

    /* Softmax selection demo */
    int valid[] = {0, 1, 2, 3, 4, 5, 6};
    srand(42);
    printf("\nSoftmax selections for state a1b2c3d4e5f6a7b8:\n");
    for (int i = 0; i < 10; i++) {
        int a = policy_select(&c4_policy, "a1b2c3d4e5f6a7b8", valid, 7);
        printf("  Selection %d: column %d\n", i + 1, a);
    }

    printf("\nPolicy has %d entries, temperature = %.2f\n",
           c4_policy.count, c4_policy.temperature);
    return 0;
}
