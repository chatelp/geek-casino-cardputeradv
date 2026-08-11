// Le parcours utilisateur entier, sans écran : premier lancement, joueurs,
// navigation, réglages, reset. C'est le contrat des touches de Pierre.
#include <unity.h>

#include "app.h"

void setUp() {}
void tearDown() {}

using core::App;
using core::AppKey;
using core::AppScreen;

static void typeName(App& a, const char* s) {
    for (int i = 0; s[i]; ++i) core::feedNameChar(a, s[i]);
}

static void test_first_launch_asks_for_a_name() {
    core::seedXorShift(1);
    App a = core::newApp(0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::NameEntry, a.screen);

    typeName(a, "pierre");  // minuscule : la saisie doit capitaliser
    TEST_ASSERT_EQUAL_STRING("PIERRE", a.nameEntry.buf);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Lobby, a.screen);
    TEST_ASSERT_EQUAL_STRING("PIERRE", a.roster.players[0].name);
    TEST_ASSERT_EQUAL_INT32(core::kStartingCredits, a.econ.credits);
}

static void test_lobby_keys_route_to_the_right_screens() {
    core::seedXorShift(2);
    App a = core::newApp(0, core::xorShift32);
    typeName(a, "ZOE");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);

    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::GlobalSettings, a.screen);
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);

    core::handleKey(a, AppKey::Board, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Leaderboard, a.screen);
    core::handleKey(a, AppKey::Board, 0, core::xorShift32);

    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);  // SLOTS
    TEST_ASSERT_EQUAL(AppScreen::Slot, a.screen);

    core::handleKey(a, AppKey::Help, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::SlotHelp, a.screen);
    core::handleKey(a, AppKey::Help, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Slot, a.screen);

    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::SlotSettings, a.screen);
    core::handleKey(a, AppKey::Right, 0, core::xorShift32);  // GLYPHS
    TEST_ASSERT_EQUAL_UINT8(1, a.settings.slotSkin);         // → classique
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Slot, a.screen);
}

static void test_help_returns_where_it_was_opened() {
    // L'aide s'ouvre depuis l'accueil ET depuis le jeu : chaque sortie doit
    // revenir à son point d'entrée (bug signalé par Pierre : depuis
    // l'accueil, la sortie tombait sur le jeu lancé).
    core::seedXorShift(12);
    App a = core::newApp(0, core::xorShift32);
    typeName(a, "ZOE");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);

    core::handleKey(a, AppKey::Help, 0, core::xorShift32);  // depuis l'accueil
    TEST_ASSERT_EQUAL(AppScreen::SlotHelp, a.screen);
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Lobby, a.screen);          // retour accueil

    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);  // entre en jeu
    core::handleKey(a, AppKey::Help, 0, core::xorShift32);     // depuis le jeu
    core::handleKey(a, AppKey::Help, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Slot, a.screen);              // retour jeu
}

static void test_attract_spins_are_silent() {
    // La démo n'impose rien à la pièce : aucun son émis pendant tout un
    // tour de démo, du départ à l'arrêt des trois rouleaux.
    core::seedXorShift(13);
    uint32_t now = 0;
    core::Game g = core::newGame(now, core::xorShift32);
    while (core::takeCue(g) != core::Cue::None) {}
    TEST_ASSERT_TRUE(core::startSpin(g, now, core::xorShift32, /*byPlayer=*/false));
    for (int i = 0; i < 400; ++i) {
        now += core::kFrameMs;
        core::updateGame(g, now, core::xorShift32);
        TEST_ASSERT_EQUAL(core::Cue::None, core::takeCue(g));
        if (g.phase == core::Phase::Idle && i > 10) break;
    }
}

static void test_esc_cancels_name_entry_when_a_player_exists() {
    // Bug signalé : sur l'appareil, Échap pendant la saisie était avalé par
    // le filtre de caractères. Ici on vérifie le contrat côté logique.
    core::seedXorShift(14);
    App a = core::newApp(0, core::xorShift32);
    // Premier lancement : pas d'annulation possible, il FAUT un nom.
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::NameEntry, a.screen);

    typeName(a, "ZOE");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);

    // Changement de joueur, puis on se ravise : Échap ramène à l'accueil
    // et la saisie en cours est oubliée.
    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::NameEntry, a.screen);
    typeName(a, "BOB");
    core::handleKey(a, AppKey::Back, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::Lobby, a.screen);
    TEST_ASSERT_EQUAL_UINT8(1, a.roster.count);        // BOB n'existe pas
    TEST_ASSERT_EQUAL_UINT8(0, a.nameEntry.len);       // saisie effacée
    TEST_ASSERT_EQUAL_STRING("ZOE", a.roster.players[a.roster.current].name);
}

static void test_switching_player_swaps_the_balance() {
    core::seedXorShift(3);
    App a = core::newApp(0, core::xorShift32);
    typeName(a, "ONE");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    a.econ.credits = 750;  // ONE joue et perd un peu
    core::pushEconomy(a);

    // Nouveau joueur via réglages (ligne PLAYER, Enter).
    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL(AppScreen::NameEntry, a.screen);
    typeName(a, "TWO");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);

    // TWO démarre au solde neuf ; le solde de ONE est resté à 750.
    TEST_ASSERT_EQUAL_INT32(core::kStartingCredits, a.econ.credits);
    TEST_ASSERT_EQUAL_INT32(750, a.roster.players[0].credits);

    // Retour à ONE par ←/→ sur la ligne PLAYER : son solde revient.
    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Right, 0, core::xorShift32);
    TEST_ASSERT_EQUAL_STRING("ONE", a.roster.players[a.roster.current].name);
    TEST_ASSERT_EQUAL_INT32(750, a.econ.credits);
}

static void test_same_name_switches_instead_of_duplicating() {
    core::seedXorShift(4);
    App a = core::newApp(0, core::xorShift32);
    typeName(a, "ONE");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    typeName(a, "ONE");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    TEST_ASSERT_EQUAL_UINT8(1, a.roster.count);  // pas de doublon
    TEST_ASSERT_EQUAL(AppScreen::Lobby, a.screen);
}

static void test_reset_needs_two_presses_and_wipes_everything() {
    core::seedXorShift(5);
    App a = core::newApp(0, core::xorShift32);
    typeName(a, "ONE");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);

    core::handleKey(a, AppKey::Settings, 0, core::xorShift32);
    for (int i = 0; i < 3; ++i) core::handleKey(a, AppKey::Down, 0, core::xorShift32);

    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);  // arme
    TEST_ASSERT_TRUE(a.resetArmed);
    TEST_ASSERT_EQUAL_UINT8(1, a.roster.count);  // rien d'effacé encore

    // Bouger désarme : pas d'effacement par erreur de navigation.
    core::handleKey(a, AppKey::Up, 0, core::xorShift32);
    TEST_ASSERT_FALSE(a.resetArmed);

    core::handleKey(a, AppKey::Down, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);  // confirme
    TEST_ASSERT_EQUAL_UINT8(0, a.roster.count);
    TEST_ASSERT_EQUAL(AppScreen::NameEntry, a.screen);  // comme au premier
}                                                       // lancement

static void test_spin_updates_the_leaderboard_entry() {
    core::seedXorShift(6);
    App a = core::newApp(0, core::xorShift32);
    typeName(a, "ONE");
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);
    core::handleKey(a, AppKey::Confirm, 0, core::xorShift32);  // entre en jeu

    uint32_t now = 0;
    core::handleKey(a, AppKey::Confirm, now, core::xorShift32);  // tire
    for (int i = 0; i < 400; ++i) {
        now += core::kFrameMs;
        core::tickApp(a, now, core::xorShift32);
    }
    TEST_ASSERT_EQUAL_UINT32(1, a.roster.players[0].spins);
    TEST_ASSERT_EQUAL_INT32(a.econ.credits, a.roster.players[0].credits);
    TEST_ASSERT_TRUE(a.dirty);  // une sauvegarde est due
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_first_launch_asks_for_a_name);
    RUN_TEST(test_lobby_keys_route_to_the_right_screens);
    RUN_TEST(test_help_returns_where_it_was_opened);
    RUN_TEST(test_attract_spins_are_silent);
    RUN_TEST(test_esc_cancels_name_entry_when_a_player_exists);
    RUN_TEST(test_switching_player_swaps_the_balance);
    RUN_TEST(test_same_name_switches_instead_of_duplicating);
    RUN_TEST(test_reset_needs_two_presses_and_wipes_everything);
    RUN_TEST(test_spin_updates_the_leaderboard_entry);
    return UNITY_END();
}
