/**
 * benchmark.c — Performance benchmark for compiled policy operations.
 *
 * Targets: lookup <100ns, hash <500ns, full decision <1µs
 */

#define _POSIX_C_SOURCE 199309L

#include <stdio.h>
#include <string.h>
#include <time.h>
#include "compiled_policy.h"
#include "ttt_policy_data.h"

#define NS_PER_SEC 1000000000ULL

static inline uint64_t now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * NS_PER_SEC + (uint64_t)ts.tv_nsec;
}

int main(void) {
    printf("=== Compiled Policy Benchmarks ===\n");
    printf("Version: %s\n\n", policy_version());

    const int N = 1000000;
    char hash[17];
    volatile int sink = 0;  /* prevent optimization */

    /* --- Hash benchmark --- */
    {
        uint64_t t0 = now_ns();
        for (int i = 0; i < N; i++) {
            policy_hash("         ", hash);
            sink += hash[0];
        }
        uint64_t t1 = now_ns();
        double ns_per = (double)(t1 - t0) / N;
        printf("policy_hash:     %8.1f ns/op  (target: <500ns) %s\n",
               ns_per, ns_per < 500 ? "✓" : "✗");
    }

    /* Build hash table first */
    policy_lookup(&ttt_policy, "5488becc4be28c12");

    /* --- Lookup benchmark --- */
    {
        const char* known_hash = ttt_policy.entries[0].hash;
        uint64_t t0 = now_ns();
        for (int i = 0; i < N; i++) {
            sink += policy_lookup(&ttt_policy, known_hash);
        }
        uint64_t t1 = now_ns();
        double ns_per = (double)(t1 - t0) / N;
        printf("policy_lookup:   %8.1f ns/op  (target: <100ns) %s\n",
               ns_per, ns_per < 100 ? "✓" : "✗");
    }

    /* --- Full decision (hash + lookup) --- */
    {
        uint64_t t0 = now_ns();
        for (int i = 0; i < N; i++) {
            policy_hash("         ", hash);
            sink += policy_lookup(&ttt_policy, hash);
        }
        uint64_t t1 = now_ns();
        double ns_per = (double)(t1 - t0) / N;
        printf("full decision:   %8.1f ns/op  (target: <1µs)  %s\n",
               ns_per, ns_per < 1000 ? "✓" : "✗");
    }

    /* --- policy_select benchmark --- */
    {
        int valid[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
        policy_hash("         ", hash);
        uint64_t t0 = now_ns();
        for (int i = 0; i < N; i++) {
            sink += policy_select(&ttt_policy, hash, valid, 9);
        }
        uint64_t t1 = now_ns();
        double ns_per = (double)(t1 - t0) / N;
        printf("policy_select:   %8.1f ns/op\n", ns_per);
    }

    /* --- Memory footprint --- */
    {
        size_t entry_size = sizeof(PolicyEntry);
        size_t policy_size = sizeof(CompiledPolicy);
        size_t data_size = entry_size * ttt_policy.count;
        printf("\nMemory footprint:\n");
        printf("  PolicyEntry:   %zu bytes\n", entry_size);
        printf("  TTT entries:   %zu bytes (%d entries)\n", data_size, ttt_policy.count);
        printf("  CompiledPolicy: %zu bytes\n", policy_size);
        printf("  Total (TTT):   ~%zu bytes\n", data_size + policy_size);
        printf("  Hash table:    %d buckets + chains\n", 256);
    }

    (void)sink;
    printf("\nDone.\n");
    return 0;
}
