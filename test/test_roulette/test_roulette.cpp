// La propriété remarquable de la roulette européenne : TOUTES les mises
// ont exactement le même retour, 36/37. Si une seule diffère, la table de
// gains est fausse — et un joueur pourrait trouver la mise avantageuse.
#include <cstdio>
#include <unity.h>

#include "roulette.h"
#include "roulette_session.h"

void setUp() {}
void tearDown() {}

static void test_every_bet_has_the_same_exact_return() {
    const double expected = 36.0 / 37.0;
    for (uint8_t i = 0; i < core::kBetKinds; ++i) {
        const core::BetKind k = static_cast<core::BetKind>(i);
        // Pour le plein, on vérifie les 37 numéros, pas un seul.
        const uint8_t picks = k == core::BetKind::Straight ? core::kPockets : 1;
        for (uint8_t p = 0; p < picks; ++p) {
            const double rtp = core::exactRouletteRtp(k, p);
            const int32_t ppm = static_cast<int32_t>(rtp * 1000000.0 + 0.5);
            TEST_ASSERT_EQUAL_INT32_MESSAGE(972973, ppm, core::betName(k));
        }
    }
    std::printf("\n    RTP de chaque mise = %.4f %%\n", expected * 100.0);
}

static void test_the_wheel_is_the_real_european_sequence() {
    // Les 37 cases exactement une fois : une roue qui répète un numéro
    // fausserait toutes les probabilités sans qu'aucun gain ne semble faux.
    bool seen[core::kPockets] = {false};
    for (uint8_t i = 0; i < core::kPockets; ++i) {
        const uint8_t n = core::pocketAt(i);
        TEST_ASSERT_LESS_THAN_UINT8(core::kPockets, n);
        TEST_ASSERT_FALSE_MESSAGE(seen[n], "numero present deux fois sur la roue");
        seen[n] = true;
    }
    for (uint8_t n = 0; n < core::kPockets; ++n) TEST_ASSERT_TRUE(seen[n]);

    // L'aller-retour index/numéro doit être exact.
    for (uint8_t n = 0; n < core::kPockets; ++n) {
        TEST_ASSERT_EQUAL_UINT8(n, core::pocketAt(core::indexOfPocket(n)));
    }
    // Repères vérifiables : le 0 est en tête, le 26 ferme la roue, et le
    // 32 est bien le voisin du zéro.
    TEST_ASSERT_EQUAL_UINT8(0, core::pocketAt(0));
    TEST_ASSERT_EQUAL_UINT8(32, core::pocketAt(1));
    TEST_ASSERT_EQUAL_UINT8(26, core::pocketAt(36));

    // Sur une vraie roue, deux cases voisines sont de couleurs opposées
    // (le zéro mis à part). C'est un invariant du plan de roue.
    for (uint8_t i = 0; i < core::kPockets; ++i) {
        const uint8_t a = core::pocketAt(i);
        const uint8_t b = core::pocketAt(i + 1);
        if (a == 0 || b == 0) continue;
        TEST_ASSERT_TRUE_MESSAGE(core::isRed(a) != core::isRed(b),
                                 "deux voisines de meme couleur sur la roue");
    }
}

static void test_colours_match_the_layout() {
    uint8_t reds = 0, blacks = 0;
    for (uint8_t n = 1; n <= 36; ++n) {
        if (core::isRed(n)) ++reds;
        else if (core::isBlack(n)) ++blacks;
    }
    TEST_ASSERT_EQUAL_UINT8(18, reds);
    TEST_ASSERT_EQUAL_UINT8(18, blacks);
    TEST_ASSERT_FALSE(core::isRed(0));
    TEST_ASSERT_FALSE(core::isBlack(0));  // le zéro n'est ni l'un ni l'autre
}

static void test_zero_beats_every_outside_bet() {
    // C'est LÀ que la maison gagne, et nulle part ailleurs.
    for (uint8_t i = 0; i < core::kBetKinds; ++i) {
        const core::BetKind k = static_cast<core::BetKind>(i);
        if (k == core::BetKind::Straight) continue;
        TEST_ASSERT_FALSE_MESSAGE(core::betWins(k, 0, 0),
                                  "une chance simple a gagne sur le zero");
    }
    // Sauf le plein sur le zéro lui-même, qui paie 36.
    TEST_ASSERT_TRUE(core::betWins(core::BetKind::Straight, 0, 0));
}

static void test_bet_coverage_is_what_it_claims() {
    uint8_t low = 0, high = 0, d1 = 0, d2 = 0, d3 = 0;
    for (uint8_t n = 0; n < core::kPockets; ++n) {
        if (core::betWins(core::BetKind::Low, 0, n)) ++low;
        if (core::betWins(core::BetKind::High, 0, n)) ++high;
        if (core::betWins(core::BetKind::Dozen1, 0, n)) ++d1;
        if (core::betWins(core::BetKind::Dozen2, 0, n)) ++d2;
        if (core::betWins(core::BetKind::Dozen3, 0, n)) ++d3;
    }
    TEST_ASSERT_EQUAL_UINT8(18, low);
    TEST_ASSERT_EQUAL_UINT8(18, high);
    TEST_ASSERT_EQUAL_UINT8(12, d1);
    TEST_ASSERT_EQUAL_UINT8(12, d2);
    TEST_ASSERT_EQUAL_UINT8(12, d3);
    // Les trois douzaines ne se recouvrent pas et couvrent 1 à 36.
    TEST_ASSERT_EQUAL_UINT8(36, d1 + d2 + d3);
}

static void test_the_ball_bounces_then_rests_in_its_pocket() {
    // L'arrêt était trop franc : la bille se posait net et le règlement
    // tombait la même image. Elle rebondit désormais pendant Landing —
    // l'écart existe, s'amortit, et finit exactement sur la case.
    core::seedXorShift(77);
    core::RouletteSession r = core::newRouletteSession(0, core::xorShift32);
    r.econ.credits = 1000;
    TEST_ASSERT_TRUE(core::rltSpin(r, 0, core::xorShift32));

    uint32_t now = 0;
    while (r.phase == core::RltPhase::Spinning && now < 10000) {
        now += core::kFrameMs;
        core::rltUpdate(r, now);
    }
    TEST_ASSERT_EQUAL(core::RltPhase::Landing, r.phase);

    // Tôt dans le rebond : la bille n'est PAS posée sur la case.
    const float early = core::rltWheelPos(r, r.phaseT0 + 60);
    float dev = early - static_cast<float>(r.winNumber);
    if (dev < 0) dev = -dev;
    TEST_ASSERT_TRUE_MESSAGE(dev > 0.05f, "le rebond doit exister");

    // JAMAIS près de la frontière : à ±0,42 l'arrêt se lisait comme une
    // hésitation entre deux cases. Le maximum est balayé sur toute la
    // phase — pas seulement un instant — et doit rester nettement dans la
    // case.
    float worst = 0.0f;
    for (uint32_t t = 0; t <= core::kRltLandMs; t += 10) {
        float d = core::rltWheelPos(r, r.phaseT0 + t) -
                  static_cast<float>(r.winNumber);
        if (d < 0) d = -d;
        if (d > worst) worst = d;
    }
    TEST_ASSERT_TRUE_MESSAGE(worst < 0.25f,
                             "le rebond doit rester DANS la case");

    // Fin du rebond : posée exactement, avant même le règlement.
    const float late = core::rltWheelPos(r, r.phaseT0 + core::kRltLandMs);
    TEST_ASSERT_EQUAL_FLOAT(static_cast<float>(r.winNumber), late);

    // Et le règlement n'est pas encore fait : le solde n'a pas bougé
    // pendant le rebond.
    TEST_ASSERT_EQUAL(core::RltPhase::Landing, r.phase);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_every_bet_has_the_same_exact_return);
    RUN_TEST(test_the_wheel_is_the_real_european_sequence);
    RUN_TEST(test_colours_match_the_layout);
    RUN_TEST(test_zero_beats_every_outside_bet);
    RUN_TEST(test_bet_coverage_is_what_it_claims);
    RUN_TEST(test_the_ball_bounces_then_rests_in_its_pocket);
    return UNITY_END();
}
