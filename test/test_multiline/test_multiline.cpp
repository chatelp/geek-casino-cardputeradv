// Format vidéo 5×3, 5 lignes. Le RTP se calcule analytiquement : 32^5 fait
// 33 millions de combinaisons, mais l'espérance d'une ligne ne dépend que
// des effectifs de la bande. Un test de simulation vérifie que le calcul
// et le tirage réel disent la même chose.
#include <cstdio>
#include <unity.h>

#include "multiline.h"

void setUp() {}
void tearDown() {}

static void test_line_rtp_is_about_95_percent() {
    const double rtp = core::exactLineRtp(core::videoReelSet(), core::videoPaytable(),
                                          core::kVideoReels);
    const double hit = core::exactLineHitRate(core::videoReelSet(),
                                              core::videoPaytable(), core::kVideoReels);
    std::printf("\n    RTP par ligne = %.4f %%   lignes gagnantes = %.3f %%\n",
                rtp * 100.0, hit * 100.0);
    TEST_ASSERT_TRUE_MESSAGE(rtp > 0.93, "RTP trop bas");
    TEST_ASSERT_TRUE_MESSAGE(rtp < 0.97, "RTP trop haut");
    // Avec 5 lignes, la probabilité qu'au moins une paie est bien plus
    // haute : le format vidéo doit récompenser plus souvent, moins fort.
    TEST_ASSERT_TRUE(hit > 0.02);
}

static void test_paylines_stay_inside_the_grid() {
    const core::Payline* l = core::videoPaylines();
    for (uint8_t i = 0; i < core::kVideoLines; ++i) {
        for (uint8_t r = 0; r < core::kVideoReels; ++r) {
            TEST_ASSERT_LESS_THAN_UINT8(core::kVideoRows, l[i].row[r]);
        }
    }
    // Ligne 0 = le centre : c'est celle qu'on montre pour expliquer.
    for (uint8_t r = 0; r < core::kVideoReels; ++r) {
        TEST_ASSERT_EQUAL_UINT8(1, l[0].row[r]);
    }
    // Aucune ligne en double : deux lignes identiques paieraient deux fois
    // la même chose sans que le joueur puisse le voir.
    for (uint8_t a = 0; a < core::kVideoLines; ++a) {
        for (uint8_t b = static_cast<uint8_t>(a + 1); b < core::kVideoLines; ++b) {
            bool same = true;
            for (uint8_t r = 0; r < core::kVideoReels; ++r) {
                if (l[a].row[r] != l[b].row[r]) { same = false; break; }
            }
            TEST_ASSERT_FALSE_MESSAGE(same, "deux lignes de paiement identiques");
        }
    }
}

static void test_payouts_grow_with_length_and_rarity() {
    const core::MultiPaytable& pt = core::videoPaytable();
    const core::ReelSet& rs = core::videoReelSet();
    for (uint8_t s = 0; s < core::kSymbolCount; ++s) {
        // Plus long paie strictement plus.
        TEST_ASSERT_TRUE(pt.pay[s][3] < pt.pay[s][4]);
        TEST_ASSERT_TRUE(pt.pay[s][4] < pt.pay[s][5]);
        // Moins de trois alignés ne paie rien : sur 5 rouleaux, une paire
        // tomberait bien trop souvent.
        TEST_ASSERT_EQUAL_UINT16(0, pt.pay[s][1]);
        TEST_ASSERT_EQUAL_UINT16(0, pt.pay[s][2]);
        if (s > 0) {
            TEST_ASSERT_TRUE_MESSAGE(pt.pay[s][5] > pt.pay[s - 1][5],
                                     "gains non croissants avec le rang");
            TEST_ASSERT_TRUE_MESSAGE(core::countOn(rs, 0, s) <= core::countOn(rs, 0, s - 1),
                                     "un symbole mieux paye doit etre plus rare");
        }
    }
}

static void test_evaluate_reads_each_line_from_the_left() {
    const core::MultiPaytable& pt = core::videoPaytable();
    const core::Payline* lines = core::videoPaylines();
    core::GridOutcome o{};

    // Grille entièrement remplie de CHIP : les 5 lignes paient le maximum.
    for (uint8_t r = 0; r < core::kVideoReels; ++r) {
        for (uint8_t y = 0; y < core::kVideoRows; ++y) o.sym[r][y] = core::SYM_CHIP;
    }
    core::evaluateGrid(pt, lines, core::kVideoLines, core::kVideoReels, o);
    TEST_ASSERT_EQUAL_UINT8(core::kVideoLines, o.winCount);
    TEST_ASSERT_EQUAL_UINT32(5u * pt.pay[core::SYM_CHIP][5], o.totalMultiplier);
    TEST_ASSERT_FALSE(o.jackpot);

    // Trois invaders sur la ligne du centre, rien ailleurs.
    for (uint8_t r = 0; r < core::kVideoReels; ++r) {
        for (uint8_t y = 0; y < core::kVideoRows; ++y) {
            o.sym[r][y] = static_cast<uint8_t>(core::SYM_RESISTOR + (r + y) % 3 + 1);
        }
    }
    o.sym[0][1] = o.sym[1][1] = o.sym[2][1] = core::SYM_INVADER;
    o.sym[3][1] = core::SYM_LED;
    core::evaluateGrid(pt, lines, core::kVideoLines, core::kVideoReels, o);
    bool found = false;
    for (uint8_t i = 0; i < o.winCount; ++i) {
        if (o.wins[i].line == 0) {
            TEST_ASSERT_EQUAL_UINT8(core::SYM_INVADER, o.wins[i].symbol);
            TEST_ASSERT_EQUAL_UINT8(3, o.wins[i].count);
            found = true;
        }
    }
    TEST_ASSERT_TRUE_MESSAGE(found, "la ligne centrale aurait du payer");
    TEST_ASSERT_FALSE(o.jackpot);  // trois, pas cinq
}

static void test_jackpot_needs_five_invaders_on_a_line() {
    core::GridOutcome o{};
    for (uint8_t r = 0; r < core::kVideoReels; ++r) {
        for (uint8_t y = 0; y < core::kVideoRows; ++y) o.sym[r][y] = core::SYM_LED;
        o.sym[r][1] = core::SYM_INVADER;
    }
    core::evaluateGrid(core::videoPaytable(), core::videoPaylines(),
                       core::kVideoLines, core::kVideoReels, o);
    TEST_ASSERT_TRUE(o.jackpot);
}

static void test_grid_reads_three_consecutive_strip_positions() {
    const core::ReelSet& rs = core::videoReelSet();
    core::GridOutcome o{};
    for (uint8_t r = 0; r < core::kVideoReels; ++r) o.pos[r] = static_cast<uint16_t>(r * 5);
    core::fillGrid(rs, o, core::kVideoRows);
    for (uint8_t r = 0; r < core::kVideoReels; ++r) {
        for (uint8_t y = 0; y < core::kVideoRows; ++y) {
            TEST_ASSERT_EQUAL_UINT8(core::symbolAt(rs, r, o.pos[r] + y), o.sym[r][y]);
        }
    }
}

static void test_simulated_draw_matches_the_analytic_model() {
    // Le RTP simulé converge TRÈS lentement : l'écart-type d'une ligne vaut
    // 27 fois son espérance (le 15000 du jackpot écrase tout). Il faudrait
    // 1,5 million de tours pour un intervalle de ±1 point. On vérifie donc
    // deux choses de variance faible — la fréquence de gain et la
    // répartition des symboles — puis le RTP dans une bande justifiée.
    core::seedXorShift(0x5107C0DE);
    const core::ReelSet& rs = core::videoReelSet();
    const core::MultiPaytable& pt = core::videoPaytable();
    const core::Payline* lines = core::videoPaylines();

    core::GridOutcome o{};
    uint64_t total = 0;
    uint64_t winningLines = 0;
    uint32_t symSeen[core::kSymbolCount] = {0};
    constexpr int kSpins = 300000;
    constexpr int kDraws = kSpins * core::kVideoLines;

    for (int i = 0; i < kSpins; ++i) {
        core::spinGrid(rs, core::xorShift32, o, core::kVideoRows);
        core::evaluateGrid(pt, lines, core::kVideoLines, core::kVideoReels, o);
        total += o.totalMultiplier;
        winningLines += o.winCount;
        for (uint8_t r = 0; r < core::kVideoReels; ++r) {
            for (uint8_t y = 0; y < core::kVideoRows; ++y) symSeen[o.sym[r][y]]++;
        }
    }

    // 1. Fréquence de ligne gagnante : converge vite, doit coller de près.
    const double obsHit = static_cast<double>(winningLines) / kDraws;
    const double exHit = core::exactLineHitRate(rs, pt, core::kVideoReels);
    std::printf("\n    lignes gagnantes : simule %.4f %%  analytique %.4f %%\n",
                obsHit * 100.0, exHit * 100.0);
    TEST_ASSERT_TRUE_MESSAGE(obsHit > exHit * 0.94 && obsHit < exHit * 1.06,
                             "la frequence de gain ne suit pas le modele");

    // 2. Chaque symbole doit apparaître à hauteur de sa place sur la bande.
    // C'est ce qui prouve que spinGrid lit vraiment la bande vidéo.
    const uint32_t cells = static_cast<uint32_t>(kSpins) * core::kVideoReels * core::kVideoRows;
    for (uint8_t s = 0; s < core::kSymbolCount; ++s) {
        const double expected = static_cast<double>(core::countOn(rs, 0, s)) / core::kStripLen;
        const double seen = static_cast<double>(symSeen[s]) / cells;
        TEST_ASSERT_TRUE_MESSAGE(seen > expected - 0.005 && seen < expected + 0.005,
                                 "un symbole ne sort pas a sa frequence de bande");
    }

    // 3. Le RTP, dans une bande de 3 erreurs-types (soit ±6,6 points ici).
    const double observed = static_cast<double>(total) / kDraws;
    const double exact = core::exactLineRtp(rs, pt, core::kVideoReels);
    std::printf("    RTP : simule %.4f %%  analytique %.4f %%  (3 sigma = 6,6 pts)\n",
                observed * 100.0, exact * 100.0);
    TEST_ASSERT_TRUE_MESSAGE(observed > exact - 0.066 && observed < exact + 0.066,
                             "RTP simule hors de 3 sigma : le modele est faux");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_line_rtp_is_about_95_percent);
    RUN_TEST(test_paylines_stay_inside_the_grid);
    RUN_TEST(test_payouts_grow_with_length_and_rarity);
    RUN_TEST(test_evaluate_reads_each_line_from_the_left);
    RUN_TEST(test_jackpot_needs_five_invaders_on_a_line);
    RUN_TEST(test_grid_reads_three_consecutive_strip_positions);
    RUN_TEST(test_simulated_draw_matches_the_analytic_model);
    return UNITY_END();
}
