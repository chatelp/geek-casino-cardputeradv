// Les deux nouveaux jeux : rythme, économie partagée, navigation.
#include <initializer_list>
#include <unity.h>

#include "app.h"
#include "poker.h"

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

static void test_changing_bet_mid_spin_cannot_change_the_payout() {
    // Le gain doit être calculé sur la mise ENGAGÉE, pas sur celle affichée
    // au moment où les rouleaux s'arrêtent. Sinon monter la mise pendant la
    // rotation paierait plus que ce qu'on a misé.
    core::seedXorShift(77);
    core::VideoGame g = core::newVideoGame(0, core::xorShift32);
    g.econ.credits = 100000;
    g.econ.betIndex = 0;                        // mise 1 par ligne
    TEST_ASSERT_TRUE(core::startVideoSpin(g, 0, core::xorShift32));

    // Le joueur monte la mise au maximum pendant que ça tourne.
    uint32_t now = 0;
    for (int k = 0; k < core::kBetSteps; ++k) core::raiseBet(g.econ);
    const int32_t creditsBefore = g.econ.credits;
    for (int i = 0; i < 400; ++i) {
        now += core::kFrameMs;
        core::updateVideoGame(g, now, core::xorShift32);
        if (g.phase != core::Phase::Spinning) break;
    }
    // Le gain doit valoir multiplicateur x 1, pas x 50.
    TEST_ASSERT_EQUAL_UINT32(g.outcome.totalMultiplier * 1, g.payout);
    TEST_ASSERT_EQUAL_INT32(creditsBefore + static_cast<int32_t>(g.payout),
                            g.econ.credits);
}

static void test_bet_cannot_change_while_reels_turn() {
    App a = started(78);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);  // slots
    uint32_t now = 0;
    core::handleKey(a, AppKey::Confirm, now, core::xorShift32);  // tire
    const uint8_t betDuring = a.game.machine.econ.betIndex;
    core::handleKey(a, AppKey::Right, now, core::xorShift32);
    TEST_ASSERT_EQUAL_UINT8_MESSAGE(betDuring, a.game.machine.econ.betIndex,
                                    "la mise a bouge pendant la rotation");
}

static void test_video_bet_respects_the_real_cost_of_a_spin() {
    // Un tour coûte cinq mises : proposer une mise que le solde ne couvre
    // pas cinq fois serait un affichage mensonger.
    core::seedXorShift(79);
    core::VideoGame g = core::newVideoGame(0, core::xorShift32);
    g.econ.credits = 30;          // permet 5 par ligne (25), pas 10 (50)
    g.econ.betIndex = 0;
    for (int k = 0; k < core::kBetSteps; ++k) core::raiseBetFor(g.econ, core::kVideoLines);
    TEST_ASSERT_TRUE_MESSAGE(core::videoStake(g.econ) <= 30,
                             "mise affichee superieure a ce que le solde permet");
    TEST_ASSERT_EQUAL_UINT16(5, core::bet(g.econ));
}

static void test_each_game_keeps_its_own_bet() {
    // Une mise commune changerait l'enjeu à l'insu du joueur : au format
    // vidéo, 5 en engage 25.
    App a = started(90);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);   // slots
    core::handleKey(a, AppKey::Right, 0, core::xorShift32);     // mise +1
    const uint16_t slotBet = core::bet(a.game.machine.econ);
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);

    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);   // vidéo
    core::handleKey(a, AppKey::Left, 0, core::xorShift32);      // mise -1
    TEST_ASSERT_NOT_EQUAL(slotBet, core::bet(a.video.econ));
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);

    // De retour aux slots, la mise n'a pas bougé.
    core::handleKey(a, AppKey::Up, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL_UINT16(slotBet, core::bet(a.game.machine.econ));
}

static void test_each_player_keeps_their_own_bets() {
    App a = started(91);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);   // slots
    for (int i = 0; i < 3; ++i) core::handleKey(a, AppKey::Right, 0, core::xorShift32);
    const uint16_t zoeBet = core::bet(a.game.machine.econ);
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);

    // Nouveau joueur : il repart sur la mise par défaut, pas celle de ZOE.
    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    for (const char* c = "BOB"; *c; ++c) core::feedNameChar(a, *c);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL_UINT16(core::kBetLadder[core::kDefaultBetIndex],
                             core::bet(a.game.machine.econ));

    // Retour à ZOE : elle retrouve la sienne.
    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Right, 0, core::xorShift32);
    TEST_ASSERT_EQUAL_STRING("ZOE", a.roster.players[a.roster.current].name);
    TEST_ASSERT_EQUAL_UINT16(zoeBet, core::bet(a.game.machine.econ));
}

static void test_bet_memory_survives_a_save_round_trip() {
    App a = started(92);
    a.bets.bet[0][0] = 4; a.bets.bet[0][1] = 1; a.bets.bet[0][2] = 3;
    const core::BetMemory saved = core::makeBets(a.bets);
    TEST_ASSERT_TRUE(core::betsValid(saved));

    App b = started(93);
    TEST_ASSERT_TRUE(core::betsValid(saved));
    b.bets = saved;
    core::loadPlayerBets(b);
    TEST_ASSERT_EQUAL_UINT8(4, b.game.machine.econ.betIndex);
    TEST_ASSERT_EQUAL_UINT8(1, b.video.econ.betIndex);
    TEST_ASSERT_EQUAL_UINT8(3, b.bj.econ.betIndex);

    // Un octet corrompu invalide le bloc : les mises par défaut reprennent.
    core::BetMemory bad = saved;
    bad.bet[2][1] = 99;
    TEST_ASSERT_FALSE(core::betsValid(bad));
}

static void test_poker_hand_runs_deal_hold_draw() {
    core::seedXorShift(60);
    core::VpSession s = core::newVpSession(0);
    s.econ.credits = 1000;
    const int32_t before = s.econ.credits;
    TEST_ASSERT_TRUE(core::vpDeal(s, 0, core::xorShift32));
    TEST_ASSERT_EQUAL_INT32(before - core::bet(s.econ), s.econ.credits);
    TEST_ASSERT_EQUAL(core::VpPhase::Holding, s.phase);

    // Les cinq cartes arrivent une à une : pas d'action avant la fin.
    uint32_t now = 0;
    s.cursor = core::kVpDrawSlot;
    core::vpConfirm(s, now, core::xorShift32);
    TEST_ASSERT_EQUAL_MESSAGE(core::VpPhase::Holding, s.phase,
                              "on a pu tirer avant la fin de la donne");
    for (int i = 0; i < 40; ++i) {
        now += core::kFrameMs;
        core::vpUpdate(s, now, core::xorShift32);
    }
    TEST_ASSERT_EQUAL_UINT8(core::kPokerHandSize, core::vpVisible(s));

    // On garde les deux premières, on note leur identité, on tire.
    s.cursor = 0; core::vpConfirm(s, now, core::xorShift32);
    s.cursor = 1; core::vpConfirm(s, now, core::xorShift32);
    TEST_ASSERT_TRUE(s.held[0] && s.held[1] && !s.held[2]);
    const core::Card k0 = s.hand.c[0], k1 = s.hand.c[1];
    const core::Card old2 = s.hand.c[2];

    s.cursor = core::kVpDrawSlot;
    core::vpConfirm(s, now, core::xorShift32);
    TEST_ASSERT_EQUAL(core::VpPhase::Result, s.phase);
    // Les cartes gardées n'ont pas bougé.
    TEST_ASSERT_EQUAL_UINT8(k0.rank, s.hand.c[0].rank);
    TEST_ASSERT_EQUAL_UINT8(k0.suit, s.hand.c[0].suit);
    TEST_ASSERT_EQUAL_UINT8(k1.rank, s.hand.c[1].rank);
    // La troisième a été remplacée par une carte du MÊME jeu, donc jamais
    // une carte déjà en main.
    (void)old2;
    for (int i = 0; i < core::kPokerHandSize; ++i) {
        for (int j = i + 1; j < core::kPokerHandSize; ++j) {
            const bool same = s.hand.c[i].rank == s.hand.c[j].rank &&
                              s.hand.c[i].suit == s.hand.c[j].suit;
            TEST_ASSERT_FALSE_MESSAGE(same, "carte en double dans la main");
        }
    }
    TEST_ASSERT_EQUAL_UINT32(
        static_cast<uint32_t>(core::pokerPayout(s.result, s.maxBet)) * s.stake,
        s.payout);
}

static void test_poker_cursor_wraps_over_the_draw_slot() {
    core::seedXorShift(61);
    core::VpSession s = core::newVpSession(0);
    core::vpDeal(s, 0, core::xorShift32);
    s.cursor = 0;
    core::vpMoveCursor(s, -1);
    TEST_ASSERT_EQUAL_UINT8(core::kVpDrawSlot, s.cursor);  // boucle sur DRAW
    core::vpMoveCursor(s, 1);
    TEST_ASSERT_EQUAL_UINT8(0, s.cursor);
}

static void test_lobby_now_offers_four_games() {
    App a = started(62);
    TEST_ASSERT_EQUAL_UINT8(4, core::kGameCount);
    for (int i = 0; i < 3; ++i) core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    TEST_ASSERT_EQUAL_UINT8(3, a.lobbyIndex);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Poker, a.screen);
    // On ne quitte pas au milieu d'une main : la mise est engagée.
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);  // distribue
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);
    TEST_ASSERT_EQUAL_MESSAGE(AppScreen::Poker, a.screen,
                              "on a pu quitter avec une mise engagee");
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
    RUN_TEST(test_changing_bet_mid_spin_cannot_change_the_payout);
    RUN_TEST(test_bet_cannot_change_while_reels_turn);
    RUN_TEST(test_video_bet_respects_the_real_cost_of_a_spin);
    RUN_TEST(test_each_game_keeps_its_own_bet);
    RUN_TEST(test_each_player_keeps_their_own_bets);
    RUN_TEST(test_bet_memory_survives_a_save_round_trip);
    RUN_TEST(test_poker_hand_runs_deal_hold_draw);
    RUN_TEST(test_poker_cursor_wraps_over_the_draw_slot);
    RUN_TEST(test_lobby_now_offers_four_games);
    RUN_TEST(test_blackjack_deal_is_progressive_then_playable);
    RUN_TEST(test_blackjack_hand_always_terminates);
    RUN_TEST(test_basic_strategy_matches_the_reference_table);
    return UNITY_END();
}
