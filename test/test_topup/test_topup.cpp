// Le rachat de jetons — la rangée de chiffres, partout dans l'app.
//
// Ce qui se teste ici n'est pas le dessin de la pluie, c'est le CONTRAT :
// les montants, la transversalité (tous les écrans de jeu, l'accueil, les
// réglages), les deux exceptions (saisie du nom, allumage), le cumul, le
// son, la persistance, et l'extinction de l'animation.
#include <unity.h>

#include "app.h"

void setUp() {}
void tearDown() {}

namespace {

core::App started(uint32_t seed) {
    core::seedXorShift(seed);
    core::App a = core::newApp(0, core::xorShift32);
    core::addOrSwitchPlayer(a.roster, "PIXEL");
    core::enterFromSave(a);
    return a;
}

}  // namespace

static void test_each_digit_pays_its_amount() {
    // 1 → 10 ... 9 → 90, et 0 → 100 : la rangée entière, dans l'ordre du
    // clavier.
    for (uint8_t d = 1; d <= 9; ++d) {
        TEST_ASSERT_EQUAL_INT32(d * 10, core::topupAmount(d));
    }
    TEST_ASSERT_EQUAL_INT32(100, core::topupAmount(0));
}

static void test_topup_works_on_every_game_screen() {
    // Transversal veut dire PARTOUT : l'accueil, les cinq jeux, les
    // réglages, le classement. Un écran oublié serait le poker muet de
    // D-030 — la classe de bug où rien ne plante.
    static const core::AppScreen kScreens[] = {
        core::AppScreen::Lobby,      core::AppScreen::Slot,
        core::AppScreen::Video,      core::AppScreen::Blackjack,
        core::AppScreen::Poker,      core::AppScreen::Roulette,
        core::AppScreen::GlobalSettings, core::AppScreen::Leaderboard,
        core::AppScreen::About,      core::AppScreen::AppHelp,
    };
    for (const core::AppScreen sc : kScreens) {
        core::App a = started(500);
        a.screen = sc;
        const int32_t before = a.econ.credits;
        TEST_ASSERT_TRUE(core::topupKey(a, 5, 100));
        TEST_ASSERT_EQUAL_INT32(before + 50, a.econ.credits);
        TEST_ASSERT_TRUE(a.dirty);  // le rachat se sauvegarde
    }
}

static void test_topup_reaches_the_game_economy_mid_hand() {
    // Le solde du jeu affiché fait foi : les jetons ajoutés pendant une
    // main doivent arriver DANS l'économie du jeu, pas dans une copie.
    core::App a = started(501);
    a.screen = core::AppScreen::Slot;
    core::handleKey(a, core::AppKey::Confirm, 0, core::xorShift32);  // spin
    const int32_t inGame = a.game.machine.econ.credits;
    core::topupKey(a, 0, 50);
    TEST_ASSERT_EQUAL_INT32(inGame + 100, a.game.machine.econ.credits);
}

static void test_digits_type_during_name_entry_and_skip_nothing() {
    // Pendant la saisie du nom, les chiffres ÉCRIVENT : pas de rachat.
    core::App a = started(502);
    a.roster.count = 0;
    a.screen = core::AppScreen::NameEntry;
    const int32_t before = a.econ.credits;
    TEST_ASSERT_FALSE(core::topupKey(a, 5, 100));
    TEST_ASSERT_EQUAL_INT32(before, a.econ.credits);
    // Pendant l'allumage non plus : toute touche est déjà « sauter ».
    a.screen = core::AppScreen::Boot;
    TEST_ASSERT_FALSE(core::topupKey(a, 5, 100));
}

static void test_presses_pile_up_in_one_panel() {
    // Marteler la touche cumule dans le même panneau, et l'animation
    // repart : la pluie ne s'interrompt pas au deuxième appui.
    core::App a = started(503);
    core::topupKey(a, 0, 1000);
    core::topupKey(a, 0, 1400);
    core::topupKey(a, 3, 1800);
    TEST_ASSERT_EQUAL_INT32(230, a.topup.shown);
    TEST_ASSERT_TRUE(a.topup.active);
    TEST_ASSERT_EQUAL_UINT32(1800, a.topup.t0);
}

static void test_the_rain_stops_by_itself() {
    core::App a = started(504);
    core::topupKey(a, 7, 2000);
    core::tickApp(a, 2000 + core::kTopupFxMs - 50, core::xorShift32);
    TEST_ASSERT_TRUE(a.topup.active);
    core::tickApp(a, 2000 + core::kTopupFxMs + 50, core::xorShift32);
    TEST_ASSERT_FALSE(a.topup.active);
    TEST_ASSERT_EQUAL_INT32(0, a.topup.shown);
}

static void test_topup_makes_a_sound_through_the_single_drain() {
    // Le son du rachat passe par takeAppCue, comme tout le reste : c'est
    // le point de vidange unique qui a réparé le poker et la roulette.
    core::App a = started(505);
    while (core::takeAppCue(a) != core::Cue::None) {}
    core::topupKey(a, 5, 100);
    TEST_ASSERT_EQUAL(static_cast<int>(core::Cue::Topup),
                      static_cast<int>(core::takeAppCue(a)));
    TEST_ASSERT_EQUAL(static_cast<int>(core::Cue::None),
                      static_cast<int>(core::takeAppCue(a)));
}

static void test_coins_rain_inside_the_screen() {
    // La pluie est pure : mêmes entrées, mêmes pièces. Et chaque pièce
    // visible est à l'écran — une pluie qui tombe à côté ne se voit pas.
    int seen = 0;
    for (uint8_t c = 0; c < core::kTopupCoins; ++c) {
        for (uint32_t age = 0; age < core::kTopupFxMs; age += 30) {
            int16_t x, y;
            uint8_t sc;
            if (!core::topupCoinAt(c, age, &x, &y, &sc)) continue;
            ++seen;
            TEST_ASSERT_TRUE(x >= 0 && x < 240);
            TEST_ASSERT_TRUE(y > -14 && y < 135);
            TEST_ASSERT_TRUE(sc == 1 || sc == 2);
            int16_t x2, y2;
            uint8_t sc2;
            TEST_ASSERT_TRUE(core::topupCoinAt(c, age, &x2, &y2, &sc2));
            TEST_ASSERT_EQUAL_INT16(x, x2);
            TEST_ASSERT_EQUAL_INT16(y, y2);
        }
    }
    // Toutes les pièces volent à un moment ou l'autre.
    TEST_ASSERT_TRUE(seen > core::kTopupCoins * 5);
}

static void test_lobby_help_is_the_transversal_one() {
    // H à l'accueil ouvre l'aide de l'OBJET — celle qui documente le
    // rachat — et en revient. Les règles d'un jeu restent dans la sienne.
    core::App a = started(506);
    core::handleKey(a, core::AppKey::Help, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(static_cast<int>(core::AppScreen::AppHelp),
                      static_cast<int>(a.screen));
    // Elle se feuillette comme les autres.
    core::handleKey(a, core::AppKey::Down, 0, core::xorShift32);
    TEST_ASSERT_EQUAL_UINT8(1, a.helpPage);
    core::handleKey(a, core::AppKey::Back, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(static_cast<int>(core::AppScreen::Lobby),
                      static_cast<int>(a.screen));
    TEST_ASSERT_EQUAL_UINT8(0, a.helpPage);
    // Et H en jeu ouvre toujours l'aide DU jeu.
    core::handleKey(a, core::AppKey::Confirm, 0, core::xorShift32);
    core::handleKey(a, core::AppKey::Help, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(static_cast<int>(core::AppScreen::SlotHelp),
                      static_cast<int>(a.screen));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_each_digit_pays_its_amount);
    RUN_TEST(test_topup_works_on_every_game_screen);
    RUN_TEST(test_topup_reaches_the_game_economy_mid_hand);
    RUN_TEST(test_digits_type_during_name_entry_and_skip_nothing);
    RUN_TEST(test_presses_pile_up_in_one_panel);
    RUN_TEST(test_the_rain_stops_by_itself);
    RUN_TEST(test_topup_makes_a_sound_through_the_single_drain);
    RUN_TEST(test_coins_rain_inside_the_screen);
    RUN_TEST(test_lobby_help_is_the_transversal_one);
    return UNITY_END();
}
