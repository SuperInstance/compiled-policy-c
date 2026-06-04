/**
 * c4_main.c — Connect-4 example using compiled policy.
 *
 * Demonstrates softmax action selection with temperature.
 */

#include <stdio.h>
#include <string.h>
#include "compiled_policy.h"
#include "c4_policy_data.h"

int main(void) {
    printf("=== Compiled C4 Policy Demo ===\n");
    printf("Policy entries: %d\n", c4_policy.count);
    printf("Temperature: %.1f\n\n", c4_policy.temperature);

    /* Empty board */
    char empty[43];
    memset(empty, ' ', 42);
    empty[42] = '\0';

    char hash[17];
    policy_hash(empty, hash);
    printf("Empty board hash: %s\n", hash);

    int action = policy_lookup(&c4_policy, hash);
    printf("Best column for empty board: %d (should be center=3)\n", action);

    /* Softmax selection */
    printf("\n--- Softmax Selection ---\n");
    int valid[] = {0, 1, 2, 3, 4, 5, 6};
    for (int trial = 0; trial < 5; trial++) {
        int sel = policy_select(&c4_policy, hash, valid, 7);
        printf("  Trial %d: selected column %d\n", trial + 1, sel);
    }

    /* Show a few hashes */
    printf("\n--- State Hashes ---\n");
    /* One move played */
    char one_move[43];
    memset(one_move, ' ', 42);
    one_move[42] = '\0';
    one_move[3] = 'X';  /* X plays center */
    policy_hash(one_move, hash);
    action = policy_lookup(&c4_policy, hash);
    printf("After X plays col 3: hash=%s, action=%d\n", hash, action);

    printf("\nDone.\n");
    return 0;
}
