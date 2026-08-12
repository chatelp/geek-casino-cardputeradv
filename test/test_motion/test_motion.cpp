// Le rythme se teste sans écran : courbe d'arrêt, cascade, machine à états.
#include <cmath>
#include <cstdio>
#include <unity.h>

#include "game.h"
#include "reel_motion.h"
#include "reels.h"

void setUp() {}
void tearDown() {}

static void test_reel_lands_exactly_on_target() {
    // L'invariant qui compte : après la durée, la position est EXACTEMENT
    // la cible. Une dérive d'un centième de symbole suffirait à afficher un
    // rouleau désaligné en permanence.
    for (uint16_t to = 0; to < core::kStripLen; ++to) {
        const core::ReelMotion m = core::armReel(1000, 2, 5, to, core::kStripLen, 3);
        const float end = core::reelPosition(m, core::reelStopMs(m));
        const float target = 5.0f + m.travel;
        TEST_ASSERT_FLOAT_WITHIN(0.0001f, target, end);
        // La position d'arrêt doit retomber sur la cible modulo la bande.
        const int32_t landed = static_cast<int32_t>(end + 0.5f) % core::kStripLen;
        TEST_ASSERT_EQUAL_INT32(to, landed);
    }
}

static void test_position_never_regresses_before_overshoot() {
    // Jusqu'au dépassement, le rouleau ne doit jamais reculer : un retour en
    // arrière visible se lirait comme un bug, pas comme de l'inertie.
    const core::ReelMotion m = core::armReel(0, 0, 0, 17, core::kStripLen, 4);
    float prev = -1.0f;
    const uint32_t limit = static_cast<uint32_t>(m.dur * core::kOvershootStart);
    for (uint32_t t = 0; t <= limit; t += 5) {
        const float p = core::reelPosition(m, t);
        TEST_ASSERT_TRUE_MESSAGE(p >= prev - 0.0001f, "le rouleau recule");
        prev = p;
    }
}

static void test_overshoot_happens_and_resolves() {
    const core::ReelMotion m = core::armReel(0, 0, 0, 9, core::kStripLen, 3);
    const float target = m.travel;
    float peak = 0;
    for (uint32_t t = 0; t <= m.dur; t += 2) {
        const float p = core::reelPosition(m, t);
        if (p > peak) peak = p;
    }
    // Il dépasse vraiment...
    TEST_ASSERT_TRUE_MESSAGE(peak > target + 0.1f, "aucun depassement : l'arret est mou");
    // ...mais pas au point de montrer franchement le symbole suivant.
    TEST_ASSERT_TRUE_MESSAGE(peak < target + 1.0f, "depassement trop grand");
    // ...et il revient exactement.
    TEST_ASSERT_FLOAT_WITHIN(0.0001f, target, core::reelPosition(m, m.dur));
}

static void test_reels_stop_in_cascade() {
    // Jamais ensemble : l'attente sur le dernier rouleau est le seul
    // suspense que la machine possède.
    uint32_t prev = 0;
    for (uint8_t r = 0; r < core::kMvpReels; ++r) {
        const core::ReelMotion m = core::armReel(0, r, 0, 0, core::kStripLen, 3);
        const uint32_t stop = core::reelStopMs(m);
        TEST_ASSERT_TRUE_MESSAGE(stop > prev, "deux rouleaux s'arretent ensemble");
        prev = stop;
    }
}

static void test_tiers_follow_the_paytable() {
    core::LineWin w = {0, 0, 0, 0, false};
    TEST_ASSERT_EQUAL(core::Tier::None, core::tierOf(w));
    w.multiplier = 2;  w.count = 2;
    TEST_ASSERT_EQUAL(core::Tier::Small, core::tierOf(w));
    w.multiplier = 20;
    TEST_ASSERT_EQUAL(core::Tier::Mid, core::tierOf(w));
    w.multiplier = 250;
    TEST_ASSERT_EQUAL(core::Tier::Big, core::tierOf(w));
    w.multiplier = 1200; w.jackpot = true;
    TEST_ASSERT_EQUAL(core::Tier::Jackpot, core::tierOf(w));

    // L'escalade doit être strictement croissante, sinon un gros gain
    // durerait moins qu'un petit.
    TEST_ASSERT_TRUE(core::celebrateMs(core::Tier::None) < core::celebrateMs(core::Tier::Small));
    TEST_ASSERT_TRUE(core::celebrateMs(core::Tier::Small) < core::celebrateMs(core::Tier::Mid));
    TEST_ASSERT_TRUE(core::celebrateMs(core::Tier::Mid) < core::celebrateMs(core::Tier::Big));
    TEST_ASSERT_TRUE(core::celebrateMs(core::Tier::Big) < core::celebrateMs(core::Tier::Jackpot));
}

static void test_game_returns_to_idle_and_shows_the_outcome() {
    core::seedXorShift(4242);
    uint32_t now = 0;
    core::Game g = core::newGame(now, core::xorShift32);
    TEST_ASSERT_EQUAL(core::Phase::Idle, g.phase);

    TEST_ASSERT_TRUE(core::startSpin(g, now, core::xorShift32));
    TEST_ASSERT_EQUAL(core::Phase::Spinning, g.phase);
    // Un second appel pendant la rotation ne doit rien relancer.
    TEST_ASSERT_FALSE(core::startSpin(g, now, core::xorShift32));

    uint8_t stops = 0;
    for (int i = 0; i < 400; ++i) {
        now += core::kFrameMs;
        stops = static_cast<uint8_t>(stops + core::updateGame(g, now, core::xorShift32));
        if (g.phase == core::Phase::Idle || g.phase == core::Phase::Celebrate ||
            g.phase == core::Phase::Bailout) {
            break;
        }
    }
    TEST_ASSERT_NOT_EQUAL(core::Phase::Spinning, g.phase);
    TEST_ASSERT_EQUAL_UINT8(core::kMvpReels, stops);  // un signal sonore par rouleau

    // Ce que l'écran affiche à l'arrêt doit être ce que la logique a tiré.
    const core::ReelSet& rs = *g.machine.reels;
    for (uint8_t r = 0; r < rs.reels; ++r) {
        const float shown = core::reelDisplayPos(g, r, now);
        TEST_ASSERT_EQUAL_UINT8(g.outcome.sym[r],
                                core::symbolAt(rs, r, static_cast<int32_t>(shown)));
    }
}

static void test_attract_spins_are_free() {
    // La démo joue avec de vrais tirages mais AUCUN jeton ne bouge.
    core::seedXorShift(11);
    uint32_t now = 0;
    core::Game g = core::newGame(now, core::xorShift32);
    const int32_t before = g.machine.econ.credits;
    TEST_ASSERT_TRUE(core::startSpin(g, now, core::xorShift32, /*byPlayer=*/false));
    for (int i = 0; i < 400; ++i) {
        now += core::kFrameMs;
        core::updateGame(g, now, core::xorShift32);
        if (g.phase != core::Phase::Spinning && g.phase != core::Phase::Celebrate) break;
    }
    TEST_ASSERT_EQUAL_INT32(before, g.machine.econ.credits);
}

static void test_attract_only_runs_when_the_app_arms_it() {
    // Le délai n'appartient plus au jeu : il est commun à tout l'objet et
    // réglable, donc c'est l'app qui arme. Un jeu non armé ne part JAMAIS
    // tout seul, quel que soit le temps écoulé.
    core::seedXorShift(7);
    uint32_t now = 0;
    core::Game g = core::newGame(now, core::xorShift32);
    for (int i = 0; i < 2000; ++i) {
        now += core::kFrameMs;
        core::updateGame(g, now, core::xorShift32);
    }
    TEST_ASSERT_EQUAL_MESSAGE(core::Phase::Idle, g.phase,
                              "la demo est partie sans etre armee");
    TEST_ASSERT_FALSE(g.attract);

    // Une fois armé, le tour part et il est gratuit.
    const int32_t before = g.machine.econ.credits;
    g.demoArmed = true;
    now += core::kFrameMs;
    core::updateGame(g, now, core::xorShift32);
    TEST_ASSERT_EQUAL(core::Phase::Spinning, g.phase);
    TEST_ASSERT_TRUE(g.attract);
    TEST_ASSERT_EQUAL_INT32_MESSAGE(before, g.machine.econ.credits,
                                    "la demo a coute des jetons");
}

static void test_win_countup_lands_exactly_and_never_overshoots() {
    constexpr uint32_t kPayout = 1250;
    uint32_t prev = 0;
    for (int i = 0; i <= 100; ++i) {
        const float p = static_cast<float>(i) / 100.0f;
        const uint32_t v = core::countedPayout(kPayout, p);
        TEST_ASSERT_TRUE_MESSAGE(v <= kPayout, "le compteur depasse le gain");
        TEST_ASSERT_TRUE_MESSAGE(v >= prev, "le compteur recule");
        prev = v;
    }
    TEST_ASSERT_EQUAL_UINT32(kPayout, core::countedPayout(kPayout, 1.0f));
    // Le décompte s'achève avant la fin : le joueur doit LIRE le total.
    TEST_ASSERT_EQUAL_UINT32(kPayout, core::countedPayout(kPayout, core::kCountFraction));
    TEST_ASSERT_TRUE(core::countedPayout(kPayout, 0.0f) == 0);
}

static void test_celebration_lasts_long_enough_to_be_read() {
    // Retour de Pierre : 400 ms se lisaient comme un clignotement. Aucun
    // palier gagnant ne doit durer moins d'une seconde.
    TEST_ASSERT_TRUE_MESSAGE(core::celebrateMs(core::Tier::Small) >= 1000,
                             "petit gain trop bref pour etre lu");
    TEST_ASSERT_TRUE(core::celebrateMs(core::Tier::Small) < core::celebrateMs(core::Tier::Mid));
    TEST_ASSERT_TRUE(core::celebrateMs(core::Tier::Mid) < core::celebrateMs(core::Tier::Big));
    TEST_ASSERT_TRUE(core::celebrateMs(core::Tier::Big) < core::celebrateMs(core::Tier::Jackpot));

    // La progression va bien de 0 à 1 sur la durée du palier.
    const uint32_t dur = core::celebrateMs(core::Tier::Mid);
    TEST_ASSERT_TRUE(core::celebrateProgress(core::Phase::Celebrate, core::Tier::Mid, 100, 100) < 0.01f);
    TEST_ASSERT_TRUE(core::celebrateProgress(core::Phase::Celebrate, core::Tier::Mid, 100, 100 + dur) >= 1.0f);
}

static void test_a_long_session_never_deadlocks() {
    // Trente minutes de jeu simulées : aucune phase ne doit rester coincée.
    core::seedXorShift(0x5107);
    uint32_t now = 0;
    core::Game g = core::newGame(now, core::xorShift32);
    // Le bon signe de vie n'est pas « la phase change » — la machine repasse
    // par Idle et relance dans la même image, donc l'échantillonnage rate
    // l'état intermédiaire. C'est le lancement d'un tour qui prouve qu'elle
    // n'est pas coincée.
    int spins = 0;
    uint32_t lastSpinAt = now;
    for (uint32_t i = 0; i < 30u * 60u * 1000u / core::kFrameMs; ++i) {
        now += core::kFrameMs;
        core::updateGame(g, now, core::xorShift32);
        if (g.phase == core::Phase::Idle &&
            core::startSpin(g, now, core::xorShift32)) {
            ++spins;
            lastSpinAt = now;
        }
        TEST_ASSERT_TRUE_MESSAGE(now - lastSpinAt < 10000,
                                 "aucun tour lance depuis 10 s : machine coincee");
        TEST_ASSERT_TRUE(g.machine.econ.credits >= 0);
    }
    TEST_ASSERT_TRUE_MESSAGE(spins > 100, "trop peu de tours : test non concluant");
    std::printf("\n    %d tours en 30 min simulees, solde final %d\n",
                spins, g.machine.econ.credits);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_reel_lands_exactly_on_target);
    RUN_TEST(test_position_never_regresses_before_overshoot);
    RUN_TEST(test_overshoot_happens_and_resolves);
    RUN_TEST(test_reels_stop_in_cascade);
    RUN_TEST(test_tiers_follow_the_paytable);
    RUN_TEST(test_game_returns_to_idle_and_shows_the_outcome);
    RUN_TEST(test_attract_spins_are_free);
    RUN_TEST(test_attract_only_runs_when_the_app_arms_it);
    RUN_TEST(test_win_countup_lands_exactly_and_never_overshoots);
    RUN_TEST(test_celebration_lasts_long_enough_to_be_read);
    RUN_TEST(test_a_long_session_never_deadlocks);
    return UNITY_END();
}
