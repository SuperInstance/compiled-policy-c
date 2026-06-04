/**
 * test_lookup.c — Test hash table lookup correctness.
 */

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "compiled_policy.h"
#include "ttt_policy_data.h"

static int tests_passed = 0;
static int tests_failed = 0;

#define TEST(name) printf("  TEST: %s ... ", name)
#define PASS() do { printf("PASS\n"); tests_passed++; } while(0)
#define FAIL(msg) do { printf("FAIL: %s\n", msg); tests_failed++; } while(0)

int main(void) {
    printf("=== Lookup Tests ===\n\n");

    /* Test 1: Empty board returns center (4) */
    {
        TEST("empty board → action 4");
        char hash[17];
        policy_hash("         ", hash);
        int action = policy_lookup(&ttt_policy, hash);
        if (action == 4) PASS();
        else FAIL("expected 4");
    }

    /* Test 2: Missing entry returns -1 */
    {
        TEST("unknown hash → -1");
        int action = policy_lookup(&ttt_policy, "ffffffffffffffff");
        if (action == -1) PASS();
        else FAIL("expected -1");
    }

    /* Test 3: Known position */
    {
        TEST("'O      X ' → action 5");
        char hash[17];
        policy_hash("O      X ", hash);
        int action = policy_lookup(&ttt_policy, hash);
        if (action == 5) PASS();
        else FAIL("expected 5");
    }

    /* Test 4: All entries are findable */
    {
        TEST("all entries findable");
        int ok = 1;
        for (int i = 0; i < ttt_policy.count; i++) {
            int action = policy_lookup(&ttt_policy, ttt_policy.entries[i].hash);
            if (action != ttt_policy.entries[i].action) {
                printf("\n    entry %d: expected %d got %d",
                       i, ttt_policy.entries[i].action, action);
                ok = 0;
            }
        }
        if (ok) PASS();
        else FAIL("some entries not found");
    }

    /* Test 5: Repeated lookups are consistent */
    {
        TEST("repeated lookups consistent");
        char hash[17];
        policy_hash("         ", hash);
        int a1 = policy_lookup(&ttt_policy, hash);
        int a2 = policy_lookup(&ttt_policy, hash);
        int a3 = policy_lookup(&ttt_policy, hash);
        if (a1 == a2 && a2 == a3 && a1 == 4) PASS();
        else FAIL("inconsistent results");
    }

    /* Test 6: policy_select with valid moves */
    {
        TEST("policy_select returns valid action");
        char hash[17];
        policy_hash("         ", hash);
        int valid[] = {0, 1, 2, 3, 4, 5, 6, 7, 8};
        int action = policy_select(&ttt_policy, hash, valid, 9);
        int found = 0;
        for (int i = 0; i < 9; i++) {
            if (valid[i] == action) found = 1;
        }
        if (found) PASS();
        else FAIL("returned invalid action");
    }

    /* Test 7: policy_select with empty valid */
    {
        TEST("policy_select empty valid → -1");
        int action = policy_select(&ttt_policy, "ffffffffffffffff", NULL, 0);
        if (action == -1) PASS();
        else FAIL("expected -1");
    }

    printf("\n=== Results: %d passed, %d failed ===\n",
           tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
