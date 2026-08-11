#include <unity.h>

#include "players.h"

void setUp() {}
void tearDown() {}

static void test_names_are_validated() {
    TEST_ASSERT_TRUE(core::nameValid("A"));
    TEST_ASSERT_TRUE(core::nameValid("PIERRE42"));
    TEST_ASSERT_FALSE(core::nameValid(""));
    TEST_ASSERT_FALSE(core::nameValid("TOOLONG42"));   // 9 caractères
    TEST_ASSERT_FALSE(core::nameValid("lower"));       // la saisie capitalise
    TEST_ASSERT_FALSE(core::nameValid("A B"));         // pas d'espace
}

static void test_roster_is_bounded() {
    core::Roster r;
    const char* names[] = {"P1", "P2", "P3", "P4", "P5", "P6", "P7", "P8"};
    for (const char* n : names) TEST_ASSERT_TRUE(core::addOrSwitchPlayer(r, n));
    TEST_ASSERT_EQUAL_UINT8(8, r.count);
    // Plein : un nom NOUVEAU est refusé, un nom EXISTANT bascule encore.
    TEST_ASSERT_FALSE(core::addOrSwitchPlayer(r, "P9"));
    TEST_ASSERT_TRUE(core::addOrSwitchPlayer(r, "P3"));
    TEST_ASSERT_EQUAL_UINT8(2, r.current);
}

static void test_ranking_sorts_by_credits_then_best_win() {
    core::Roster r;
    core::addOrSwitchPlayer(r, "LOW");
    core::addOrSwitchPlayer(r, "HIGH");
    core::addOrSwitchPlayer(r, "TIE");
    r.players[0].credits = 100;
    r.players[1].credits = 9000;
    r.players[2].credits = 100;
    r.players[2].bestWin = 50;  // départage : TIE devant LOW

    uint8_t order[core::kMaxPlayers];
    core::rankPlayers(r, order);
    TEST_ASSERT_EQUAL_UINT8(1, order[0]);  // HIGH
    TEST_ASSERT_EQUAL_UINT8(2, order[1]);  // TIE
    TEST_ASSERT_EQUAL_UINT8(0, order[2]);  // LOW
}

static void test_reset_empties_the_board() {
    core::Roster r;
    core::addOrSwitchPlayer(r, "A");
    core::addOrSwitchPlayer(r, "B");
    core::resetRoster(r);
    TEST_ASSERT_EQUAL_UINT8(0, r.count);
    TEST_ASSERT_NULL(core::currentPlayer(r));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_names_are_validated);
    RUN_TEST(test_roster_is_bounded);
    RUN_TEST(test_ranking_sorts_by_credits_then_best_win);
    RUN_TEST(test_reset_empties_the_board);
    return UNITY_END();
}
