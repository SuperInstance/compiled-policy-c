/**
 * ttt_main.c — Tic-Tac-Toe example using compiled policy.
 *
 * Demonstrates lookup and hash computation for TTT.
 */

#include <stdio.h>
#include <string.h>
#include "compiled_policy.h"
#include "ttt_policy_data.h"

int main(void) {
    printf("=== Compiled TTT Policy Demo ===\n");
    printf("Version: %s\n", policy_version());
    printf("Policy entries: %d\n\n", ttt_policy.count);

    /* Test exact lookup on empty board */
    const char* empty_board = "         ";
    char hash[17];
    policy_hash(empty_board, hash);

    printf("Empty board hash: %s\n", hash);

    int action = policy_lookup(&ttt_policy, hash);
    printf("Best move for empty board: position %d (center)\n", action);

    /* Test a few more positions */
    const char* boards[] = {
        "X O X O  ",  /* X should play winning move */
        "  X  O   ",  /* mid-game */
        "XOXOXOXOX",  /* full board — shouldn't crash */
    };
    const char* labels[] = {
        "X O X O  ",
        "  X  O   ",
        "XOXOXOXOX (full)",
    };

    for (int i = 0; i < 3; i++) {
        policy_hash(boards[i], hash);
        action = policy_lookup(&ttt_policy, hash);
        printf("\nBoard [%s] → hash=%s → action=%d\n",
               labels[i], hash, action);
        if (action < 0) {
            printf("  (not in table — fallback needed)\n");
        }
    }

    /* Show hash speed */
    printf("\n--- Hash Demo ---\n");
    const char* states[] = {"         ", "X        ", "XO       "};
    for (int i = 0; i < 3; i++) {
        policy_hash(states[i], hash);
        printf("state=\"%s\" → %s\n", states[i], hash);
    }

    printf("\nDone.\n");
    return 0;
}
