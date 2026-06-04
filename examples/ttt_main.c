/*
 * ttt_main.c — Play tic-tac-toe using the compiled policy.
 *
 * The AI plays X and uses the compiled lookup table for moves.
 * Opponent (O) plays randomly.
 */
#define COMPILED_POLICY_IMPLEMENTATION
#include "../include/compiled_policy.h"
#include "ttt_policy.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Tic-tac-toe board as 9-char string */
typedef struct {
    char board[10]; /* 9 + null */
    int  moves;
} TTTGame;

static void ttt_init(TTTGame* g) {
    memset(g->board, ' ', 9);
    g->board[9] = '\0';
    g->moves = 0;
}

static int ttt_check_win(const char* b, char player) {
    static const int lines[8][3] = {
        {0,1,2},{3,4,5},{6,7,8}, /* rows */
        {0,3,6},{1,4,7},{2,5,8}, /* cols */
        {0,4,8},{2,4,6}          /* diags */
    };
    for (int i = 0; i < 8; i++)
        if (b[lines[i][0]] == player && b[lines[i][1]] == player && b[lines[i][2]] == player)
            return 1;
    return 0;
}

static void ttt_print(const char* b) {
    printf("\n");
    for (int r = 0; r < 3; r++) {
        printf(" %c | %c | %c\n", b[r*3], b[r*3+1], b[r*3+2]);
        if (r < 2) printf("---+---+---\n");
    }
    printf("\n");
}

int main(void) {
    srand((unsigned)time(NULL));

    /* Build hash table for fast lookup */
    int buckets[256];
    memset(buckets, -1, sizeof(buckets));
    PolicyHashTable ht = { .buckets = buckets, .nbuckets = 256 };
    policy_build_hash_table(&ht, &ttt_policy);

    int wins = 0, draws = 0, losses = 0;
    int num_games = 1000;

    printf("=== Compiled TTT Policy — %d games ===\n\n", num_games);

    for (int g = 0; g < num_games; g++) {
        TTTGame game;
        ttt_init(&game);

        while (game.moves < 9) {
            char player = (game.moves % 2 == 0) ? 'X' : 'O';

            if (player == 'X') {
                /* AI: hash state, look up best action */
                char hash[17];
                policy_hash_state(game.board, hash);
                int action = policy_lookup_hashed(&ht, &ttt_policy, hash);

                if (action < 0 || action > 8 || game.board[action] != ' ') {
                    /* Fallback: first empty cell */
                    for (int i = 0; i < 9; i++) {
                        if (game.board[i] == ' ') { action = i; break; }
                    }
                }
                game.board[action] = player;
            } else {
                /* Random opponent */
                int empty[9], ne = 0;
                for (int i = 0; i < 9; i++)
                    if (game.board[i] == ' ') empty[ne++] = i;
                if (ne == 0) break;
                game.board[empty[rand() % ne]] = player;
            }
            game.moves++;

            if (ttt_check_win(game.board, player)) {
                if (player == 'X') wins++;
                else losses++;
                break;
            }
        }
        if (game.moves == 9 && !ttt_check_win(game.board, 'X') && !ttt_check_win(game.board, 'O'))
            draws++;
    }

    printf("Results over %d games:\n", num_games);
    printf("  X wins:  %d (%.1f%%)\n", wins, 100.0 * wins / num_games);
    printf("  Draws:   %d (%.1f%%)\n", draws, 100.0 * draws / num_games);
    printf("  O wins:  %d (%.1f%%)\n", losses, 100.0 * losses / num_games);

    /* Show a sample game */
    printf("\n--- Sample Game ---\n");
    TTTGame demo;
    ttt_init(&demo);
    int first = 1;
    while (demo.moves < 9) {
        char player = (demo.moves % 2 == 0) ? 'X' : 'O';
        if (player == 'X') {
            char hash[17];
            policy_hash_state(demo.board, hash);
            int action = policy_lookup_hashed(&ht, &ttt_policy, hash);
            if (action < 0 || action > 8 || demo.board[action] != ' ') {
                for (int i = 0; i < 9; i++)
                    if (demo.board[i] == ' ') { action = i; break; }
            }
            demo.board[action] = 'X';
            if (first) { printf("Board hash: %s -> action %d\n", hash, action); first = 0; }
        } else {
            int empty[9], ne = 0;
            for (int i = 0; i < 9; i++)
                if (demo.board[i] == ' ') empty[ne++] = i;
            demo.board[empty[rand() % ne]] = 'O';
        }
        demo.moves++;
        ttt_print(demo.board);
        if (ttt_check_win(demo.board, 'X')) { printf("X wins!\n"); break; }
        if (ttt_check_win(demo.board, 'O')) { printf("O wins!\n"); break; }
    }
    if (demo.moves == 9) printf("Draw!\n");

    return 0;
}
