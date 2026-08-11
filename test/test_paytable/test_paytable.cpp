// Le test qui compte : le RTP est calculé EXACTEMENT, par énumération des
// 32^3 combinaisons de la bande. Pas de simulation, pas d'intervalle de
// confiance, pas de graine à choisir — un nombre déterministe.
#include <cstdio>
#include <unity.h>

#include "machine.h"
#include "paytable.h"
#include "reels.h"

void setUp() {}
void tearDown() {}

static void test_rtp_is_about_95_percent() {
    const double rtp = core::exactRtp(core::mvpReelSet(), core::mvpPaytable());
    std::printf("\n    RTP exact = %.4f %%\n", rtp * 100.0);
    // Fourchette étroite assumée : hors de là, l'équilibrage a bougé et la
    // décision D-006 (« RTP réaliste ~95 % ») n'est plus tenue.
    TEST_ASSERT_TRUE_MESSAGE(rtp > 0.93, "RTP trop bas — la machine est avare");
    TEST_ASSERT_TRUE_MESSAGE(rtp < 0.97, "RTP trop haut — la machine est généreuse");
}

static void test_hit_rate_is_playable() {
    const double hit = core::exactHitRate(core::mvpReelSet(), core::mvpPaytable());
    std::printf("    Tours gagnants = %.2f %%\n", hit * 100.0);
    // Trop rare, le joueur décroche ; trop fréquent, un gain ne vaut plus rien.
    TEST_ASSERT_TRUE(hit > 0.10);
    TEST_ASSERT_TRUE(hit < 0.35);
}

static void test_payouts_increase_with_rarity() {
    // Invariant de conception : un symbole plus rare paie strictement plus.
    // Si l'art réordonne les symboles, ce test tombe avant que l'injustice
    // n'atteigne le joueur.
    const core::ReelSet& rs = core::mvpReelSet();
    const core::Paytable& pt = core::mvpPaytable();
    for (uint8_t s = 1; s < core::kSymbolCount; ++s) {
        TEST_ASSERT_TRUE_MESSAGE(pt.three[s] > pt.three[s - 1],
                                 "gains non croissants avec le rang");
        TEST_ASSERT_TRUE_MESSAGE(core::countOn(rs, 0, s) <= core::countOn(rs, 0, s - 1),
                                 "un symbole mieux payé doit etre plus rare");
    }
}

static void test_jackpot_is_the_invader_and_pays_most() {
    const core::Paytable& pt = core::mvpPaytable();
    for (uint8_t s = 0; s < core::kSymbolCount; ++s) {
        if (s != core::kJackpotSymbol) {
            TEST_ASSERT_TRUE(pt.three[s] < pt.three[core::kJackpotSymbol]);
        }
    }
    const uint8_t three[3] = {core::SYM_INVADER, core::SYM_INVADER, core::SYM_INVADER};
    const core::WinResult w = core::evaluate(pt, three, 3);
    TEST_ASSERT_TRUE(w.jackpot);
    TEST_ASSERT_EQUAL_UINT8(3, w.matched);
}

static void test_evaluate_reads_from_the_left() {
    const core::Paytable& pt = core::mvpPaytable();

    const uint8_t win3[3] = {core::SYM_CHIP, core::SYM_CHIP, core::SYM_CHIP};
    TEST_ASSERT_EQUAL_UINT16(pt.three[core::SYM_CHIP], core::evaluate(pt, win3, 3).multiplier);

    const uint8_t win2[3] = {core::SYM_CHIP, core::SYM_CHIP, core::SYM_LED};
    const core::WinResult w2 = core::evaluate(pt, win2, 3);
    TEST_ASSERT_EQUAL_UINT16(pt.two, w2.multiplier);
    TEST_ASSERT_EQUAL_UINT8(2, w2.matched);

    // Deux symboles identiques mais PAS en tête : rien. C'est la règle des
    // machines réelles, et elle doit être visible dans le test.
    const uint8_t tail[3] = {core::SYM_LED, core::SYM_CHIP, core::SYM_CHIP};
    TEST_ASSERT_EQUAL_UINT16(0, core::evaluate(pt, tail, 3).multiplier);

    const uint8_t lose[3] = {core::SYM_LED, core::SYM_CHIP, core::SYM_D20};
    const core::WinResult wl = core::evaluate(pt, lose, 3);
    TEST_ASSERT_EQUAL_UINT16(0, wl.multiplier);
    TEST_ASSERT_EQUAL_UINT8(0, wl.matched);
    TEST_ASSERT_FALSE(wl.jackpot);
}

static void test_simulated_return_matches_exact_rtp() {
    // Vérifie que spin() consomme bien la bande : la moyenne observée doit
    // rejoindre le RTP exact. Si spin() se trompait de bande ou biaisait le
    // tirage, cet écart le révélerait.
    core::seedXorShift(0xCA51704D);
    core::Machine m = core::mvpMachine();
    const core::ReelSet& rs = core::mvpReelSet();
    const core::Paytable& pt = core::mvpPaytable();

    uint64_t staked = 0, returned = 0;
    core::SpinOutcome o;
    for (int i = 0; i < 400000; ++i) {
        m.econ.credits = 1000000;  // isole la mesure de l'économie
        m.econ.betIndex = core::kDefaultBetIndex;
        TEST_ASSERT_TRUE(core::playSpin(m, core::xorShift32, o));
        staked += o.stake;
        returned += o.payout;
    }
    const double observed = static_cast<double>(returned) / static_cast<double>(staked);
    const double exact = core::exactRtp(rs, pt);
    std::printf("    RTP simule = %.4f %% (exact %.4f %%)\n",
                observed * 100.0, exact * 100.0);
    TEST_ASSERT_TRUE_MESSAGE(observed > exact - 0.03 && observed < exact + 0.03,
                             "spin() ne suit pas la distribution de la bande");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_rtp_is_about_95_percent);
    RUN_TEST(test_hit_rate_is_playable);
    RUN_TEST(test_payouts_increase_with_rarity);
    RUN_TEST(test_jackpot_is_the_invader_and_pays_most);
    RUN_TEST(test_evaluate_reads_from_the_left);
    RUN_TEST(test_simulated_return_matches_exact_rtp);
    return UNITY_END();
}
