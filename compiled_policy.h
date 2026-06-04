/**
 * compiled_policy.h — Single-header interface for compiled tile policies.
 *
 * Zero-dependency C library for microcontroller deployment.
 * Works on ESP8266 (80KB RAM, 4MB flash), Arduino, STM32.
 *
 * Usage:
 *   #define COMPILED_POLICY_IMPLEMENTATION
 *   #include "compiled_policy.h"
 *
 * Or compile compiled_policy.c separately.
 */

#ifndef COMPILED_POLICY_H
#define COMPILED_POLICY_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ---------- Types ---------- */

/** A single entry in the compiled policy lookup table. */
typedef struct {
    const char* hash;   /* 16-char hex string (BLAKE2b-64 truncated) */
    int         action; /* best action index */
    float       score;  /* confidence / Q-value */
} PolicyEntry;

/** A compiled policy: array of entries + softmax temperature. */
typedef struct {
    const PolicyEntry* entries;
    int                count;
    float              temperature; /* softmax temperature (0 = greedy) */
} CompiledPolicy;

/* ---------- Core API ---------- */

/**
 * Look up the best action for a given state hash.
 * Returns the action index, or -1 if not found.
 * O(1) average via hash table internally.
 */
int policy_lookup(const CompiledPolicy* p, const char* hash);

/**
 * Select an action from valid moves using softmax scoring.
 * @param p        Compiled policy
 * @param hash     State hash (16 hex chars)
 * @param valid    Array of valid action indices
 * @param n_valid  Number of valid actions
 * @return         Selected action, or -1 on error
 */
int policy_select(const CompiledPolicy* p, const char* hash,
                  const int* valid, int n_valid);

/**
 * Compute a BLAKE2b-64 hash of a state string, output as 16 hex chars.
 * @param state    Input state string (e.g., "X O X O  ")
 * @param out_hex16  Output buffer, must hold at least 17 bytes (16 hex + NUL)
 */
void policy_hash(const char* state, char* out_hex16);

/* ---------- Utility ---------- */

/** Get the version string of this library. */
const char* policy_version(void);

#ifdef __cplusplus
}
#endif

#endif /* COMPILED_POLICY_H */
