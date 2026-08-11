// Les deux nouveaux jeux : rythme, économie partagée, navigation.
#include <initializer_list>
#include <unity.h>

#include "app.h"

void setUp() {}
void tearDown() {}

using core::App;
using core::AppKey;
using core::AppScreen;

static App started(uint32_t seed) {
    core::seedXorShift(seed);
    App a = core::newApp(0, core::xorShift32);
    for (const char* c = "ZOE"; *c; ++c) core::feedNameChar(a, *c);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    return a;
}

static void test_lobby_offers_three_playable_games() {
    App a = started(1);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Slot, a.screen);
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);

    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Video, a.screen);
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);

    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Blackjack, a.screen);
}

static void test_the_three_games_share_one_balance() {
    // C'est ce qui fait un casino plutôt que trois jouets : le solde suit
    // le joueur d'une table à l'autre.
    App a = started(2);
    a.econ.credits = 800;
    core::pushEconomy(a);

    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);  // vidéo
    uint32_t now = 0;
    core::handleKey(a, AppKey::Confirm, now, core::xorShift32);
    for (int i = 0; i < 400; ++i) {
        now += core::kFrameMs;
        core::tickApp(a, now, core::xorShift32);
    }
    core::handleKey(a, AppKey::Back, now, core::xorShift32);
    const int32_t afterVideo = a.econ.credits;
    TEST_ASSERT_NOT_EQUAL(800, afterVideo);

    core::handleKey(a, AppKey::Down, now, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, now, core::xorShift32);
    TEST_ASSERT_EQUAL_INT32(afterVideo, a.bj.econ.credits);
}

static void test_video_spin_costs_five_lines() {
    core::seedXorShift(3);
    core::VideoGame g = core::newVideoGame(0, core::xorShift32);
    g.econ.credits = 1000;
    g.econ.betIndex = core::kDefaultBetIndex;
    const int32_t before = g.econ.credits;
    const uint16_t perLine = core::bet(g.econ);
    TEST_ASSERT_TRUE(core::startVideoSpin(g, 0, core::xorShift32));
    // Cinq lignes engagées : le tour coûte cinq fois la mise affichée.
    TEST_ASSERT_EQUAL_INT32(before - 5 * perLine, g.econ.credits);
}

static void test_video_attract_is_free_and_silent() {
    core::seedXorShift(4);
    core::VideoGame g = core::newVideoGame(0, core::xorShift32);
    while (core::takeVideoCue(g) != core::Cue::None) {}
    const int32_t before = g.econ.credits;
    TEST_ASSERT_TRUE(core::startVideoSpin(g, 0, core::xorShift32, false));
    uint32_t now = 0;
    for (int i = 0; i < 300; ++i) {
        now += core::kFrameMs;
        core::updateVideoGame(g, now, core::xorShift32);
        TEST_ASSERT_EQUAL(core::Cue::None, core::takeVideoCue(g));
        if (g.phase == core::Phase::Idle && i > 60) break;
    }
    TEST_ASSERT_EQUAL_INT32(before, g.econ.credits);
}

static void test_blackjack_deal_is_progressive_then_playable() {
    core::seedXorShift(5);
    core::BjSession s = core::newBjSession(0);
    TEST_ASSERT_TRUE(core::bjStartHand(s, 0, core::xorShift32));
    TEST_ASSERT_EQUAL_UINT8(0, core::bjVisiblePlayer(s));
    uint32_t now = 0;
    for (int i = 0; i < 60; ++i) {
        now += core::kFrameMs;
        core::bjUpdate(s, now, core::xorShift32);
    }
    TEST_ASSERT_EQUAL_UINT8(2, core::bjVisiblePlayer(s));
    TEST_ASSERT_EQUAL_UINT8(2, core::bjVisibleDealer(s));
    if (s.bj.phase == core::BjPhase::PlayerTurn) TEST_ASSERT_TRUE(core::bjHoleHidden(s));
}

static void test_blackjack_hand_always_terminates() {
    core::seedXorShift(6);
    core::BjSession s = core::newBjSession(0);
    uint32_t now = 0;
    int done = 0;
    for (int h = 0; h < 2000; ++h) {
        s.econ.credits = 10000;
        if (!core::bjStartHand(s, now, core::xorShift32)) break;
        int guard = 0;
        while (s.bj.phase != core::BjPhase::Settle && ++guard < 400) {
            now += core::kFrameMs;
            core::bjUpdate(s, now, core::xorShift32);
            if (s.bj.phase == core::BjPhase::PlayerTurn && s.revealed >= 4) {
                s.choice = core::handValue(s.bj.player).total < 17
                    ? core::BjChoice::Hit : core::BjChoice::Stand;
                core::bjConfirm(s, now, core::xorShift32);
            }
        }
        TEST_ASSERT_TRUE_MESSAGE(guard < 400, "main sans fin");
        TEST_ASSERT_TRUE(s.econ.credits >= 0);
        ++done;
    }
    TEST_ASSERT_TRUE(done > 1900);
}

static void test_basic_strategy_matches_the_reference_table() {
    core::Hand h;
    auto mk = [&](std::initializer_list<uint8_t> r) -> const core::Hand& {
        core::handClear(h);
        for (uint8_t x : r) core::handAdd(h, core::Card{x, 0});
        return h;
    };
    auto up = [](uint8_t r) { return core::Card{r, 0}; };

    // Les cas contre-intuitifs : c'est là qu'un conseil faux nuirait.
    TEST_ASSERT_EQUAL(core::BjAction::Stand, core::bjBasicStrategy(mk({10, 6}), up(5), false));
    TEST_ASSERT_EQUAL(core::BjAction::Hit,   core::bjBasicStrategy(mk({10, 6}), up(10), false));
    TEST_ASSERT_EQUAL(core::BjAction::Hit,   core::bjBasicStrategy(mk({10, 2}), up(3), false));
    TEST_ASSERT_EQUAL(core::BjAction::Stand, core::bjBasicStrategy(mk({10, 2}), up(4), false));
    TEST_ASSERT_EQUAL(core::BjAction::Double, core::bjBasicStrategy(mk({6, 5}), up(10), true));
    TEST_ASSERT_EQUAL(core::BjAction::Hit,   core::bjBasicStrategy(mk({1, 6}), up(9), true));
    TEST_ASSERT_EQUAL(core::BjAction::Stand, core::bjBasicStrategy(mk({1, 8}), up(9), false));
    TEST_ASSERT_EQUAL(core::BjAction::Stand, core::bjBasicStrategy(mk({10, 7}), up(11), false));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_lobby_offers_three_playable_games);
    RUN_TEST(test_the_three_games_share_one_balance);
    RUN_TEST(test_video_spin_costs_five_lines);
    RUN_TEST(test_video_attract_is_free_and_silent);
    RUN_TEST(test_blackjack_deal_is_progressive_then_playable);
    RUN_TEST(test_blackjack_hand_always_terminates);
    RUN_TEST(test_basic_strategy_matches_the_reference_table);
    return UNITY_END();
}
