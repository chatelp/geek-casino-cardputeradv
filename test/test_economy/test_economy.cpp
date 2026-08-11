// Le garde-fou central du projet se teste ici : personne ne repart ruiné.
#include <unity.h>

#include "economy.h"
#include "machine.h"

void setUp() {}
void tearDown() {}

static void test_fresh_economy_can_play() {
    core::Economy e = core::freshEconomy();
    TEST_ASSERT_EQUAL_INT32(core::kStartingCredits, e.credits);
    TEST_ASSERT_EQUAL_UINT16(5, core::bet(e));
    TEST_ASSERT_TRUE(core::canSpin(e));
}

static void test_bet_ladder_respects_balance() {
    core::Economy e = core::freshEconomy();
    for (int i = 0; i < 20; ++i) core::raiseBet(e);
    TEST_ASSERT_EQUAL_UINT16(core::kBetLadder[core::kBetSteps - 1], core::bet(e));

    for (int i = 0; i < 20; ++i) core::lowerBet(e);
    TEST_ASSERT_EQUAL_UINT16(core::kBetLadder[0], core::bet(e));

    // On ne peut pas monter une mise que le solde ne couvre pas.
    e.credits = 7;
    e.betIndex = 0;
    for (int i = 0; i < 20; ++i) core::raiseBet(e);
    TEST_ASSERT_TRUE(core::bet(e) <= 7);
}

static void test_clamp_bet_follows_a_shrinking_balance() {
    core::Economy e = core::freshEconomy();
    e.betIndex = core::kBetSteps - 1;  // 50
    e.credits = 3;
    core::clampBet(e);
    TEST_ASSERT_EQUAL_UINT16(2, core::bet(e));
    TEST_ASSERT_TRUE(core::canSpin(e));
}

static void test_bailout_only_when_truly_stuck() {
    core::Economy e = core::freshEconomy();
    e.credits = 1;
    TEST_ASSERT_FALSE(core::needsBailout(e));  // peut encore miser 1

    e.credits = 0;
    TEST_ASSERT_TRUE(core::needsBailout(e));
    core::bailout(e);
    TEST_ASSERT_EQUAL_INT32(core::kBailoutCredits, e.credits);
    TEST_ASSERT_TRUE(core::canSpin(e));
}

static void test_bailout_leaves_a_rich_player_alone() {
    core::Economy e = core::freshEconomy();
    const int32_t before = e.credits;
    core::bailout(e);
    TEST_ASSERT_EQUAL_INT32(before, e.credits);
}

static void test_player_never_gets_permanently_ruined() {
    // Le test qui protège la décision D-004. On joue très longtemps avec la
    // pire stratégie possible (mise maximale en permanence) : à aucun moment
    // la machine ne doit refuser de jouer.
    core::seedXorShift(0xBADCAFE);
    core::Machine m = core::mvpMachine();
    core::SpinOutcome o;
    int bailouts = 0;
    for (int i = 0; i < 200000; ++i) {
        for (int k = 0; k < core::kBetSteps; ++k) core::raiseBet(m.econ);
        TEST_ASSERT_TRUE_MESSAGE(core::playSpin(m, core::xorShift32, o),
                                 "la machine a refuse de jouer : cul-de-sac");
        TEST_ASSERT_TRUE_MESSAGE(m.econ.credits >= 0, "solde negatif");
        if (o.bailedOut) ++bailouts;
    }
    TEST_ASSERT_TRUE_MESSAGE(bailouts > 0, "aucun renflouement : test non concluant");
}

static void test_payout_matches_stake_times_multiplier() {
    core::seedXorShift(2024);
    core::Machine m = core::mvpMachine();
    core::SpinOutcome o;
    for (int i = 0; i < 5000; ++i) {
        m.econ.credits = 100000;
        core::playSpin(m, core::xorShift32, o);
        TEST_ASSERT_EQUAL_UINT32(
            static_cast<uint32_t>(o.win.multiplier) * o.stake, o.payout);
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_fresh_economy_can_play);
    RUN_TEST(test_bet_ladder_respects_balance);
    RUN_TEST(test_clamp_bet_follows_a_shrinking_balance);
    RUN_TEST(test_bailout_only_when_truly_stuck);
    RUN_TEST(test_bailout_leaves_a_rich_player_alone);
    RUN_TEST(test_player_never_gets_permanently_ruined);
    RUN_TEST(test_payout_matches_stake_times_multiplier);
    return UNITY_END();
}
