/*
 * benchmark.c — Timing benchmark for compiled policy operations.
 */
#define COMPILED_POLICY_IMPLEMENTATION
#include "../include/compiled_policy.h"
#include "ttt_policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}

int main(void) {
    printf("=== Compiled Policy Benchmark ===\n\n");

    /* Build hash table */
    int buckets[256];
    memset(buckets, -1, sizeof(buckets));
    PolicyHashTable ht = { .buckets = buckets, .nbuckets = 256 };
    policy_build_hash_table(&ht, &ttt_policy);

    int N = 100000;

    /* 1. Hash benchmark */
    {
        const char* states[] = {
            "         ", "X O  X   ", "XOX O X  ",
            "  X  O   ", "X  O X O ", "OX X OX  ",
        };
        int nstates = sizeof(states) / sizeof(states[0]);
        char out[17];

        double t0 = now_ns();
        for (int i = 0; i < N; i++)
            policy_hash_state(states[i % nstates], out);
        double elapsed = now_ns() - t0;
        printf("Hash:       %.1f ns/state  (%d ops in %.2f ms)\n",
               elapsed / N, N, elapsed / 1e6);
    }

    /* 2. Binary search lookup */
    {
        char hashes[6][17];
        const char* states[] = {
            "         ", "X O  X   ", "XOX O X  ",
            "  X  O   ", "X  O X O ", "OX X OX  ",
        };
        for (int i = 0; i < 6; i++)
            policy_hash_state(states[i], hashes[i]);

        double t0 = now_ns();
        volatile int result;
        for (int i = 0; i < N; i++)
            result = policy_lookup(&ttt_policy, hashes[i % 6]);
        double elapsed = now_ns() - t0;
        printf("Lookup (binary): %.1f ns/state  (%d ops)\n",
               elapsed / N, N);
        (void)result;
    }

    /* 3. Hash table lookup */
    {
        char hashes[6][17];
        const char* states[] = {
            "         ", "X O  X   ", "XOX O X  ",
            "  X  O   ", "X  O X O ", "OX X OX  ",
        };
        for (int i = 0; i < 6; i++)
            policy_hash_state(states[i], hashes[i]);

        double t0 = now_ns();
        volatile int result;
        for (int i = 0; i < N; i++)
            result = policy_lookup_hashed(&ht, &ttt_policy, hashes[i % 6]);
        double elapsed = now_ns() - t0;
        printf("Lookup (hash):   %.1f ns/state  (%d ops)\n",
               elapsed / N, N);
        (void)result;
    }

    /* 4. Full decision (hash + lookup) */
    {
        const char* states[] = {
            "         ", "X O  X   ", "XOX O X  ",
            "  X  O   ", "X  O X O ", "OX X OX  ",
        };
        int nstates = sizeof(states) / sizeof(states[0]);
        char hash[17];

        double t0 = now_ns();
        volatile int result;
        for (int i = 0; i < N; i++) {
            policy_hash_state(states[i % nstates], hash);
            result = policy_lookup_hashed(&ht, &ttt_policy, hash);
        }
        double elapsed = now_ns() - t0;
        printf("Full decision:   %.1f ns/state  (%d ops)\n",
               elapsed / N, N);
        (void)result;
    }

    printf("\n--- Targets ---\n");
    printf("  Lookup: < 100 ns\n");
    printf("  Hash:   < 500 ns\n");
    printf("  Full:   < 1000 ns (1 µs)\n");

    return 0;
}
