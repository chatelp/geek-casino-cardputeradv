// Le classement des mains se vérifie EXACTEMENT : les 2 598 960 mains de
// cinq cartes d'un jeu de 52 ont des effectifs connus depuis des siècles.
// Si un seul diffère, le classement est faux quelque part.
#include <cstdio>
#include <initializer_list>
#include <unity.h>

#include "poker.h"

void setUp() {}
void tearDown() {}

static core::Hand mk(std::initializer_list<int> cards) {
    core::Hand h;
    core::handClear(h);
    // Chaque entrée est un index 0..51 : rang = i%13+1, couleur = i/13.
    for (int i : cards) {
        core::handAdd(h, core::Card{static_cast<uint8_t>(i % 13 + 1),
                                    static_cast<uint8_t>(i / 13)});
    }
    return h;
}

static void test_every_possible_hand_is_classified() {
    // Effectifs de référence, jeu unique, cinq cartes.
    const uint32_t expected[core::kPokerRankCount] = {
        /* rien / petite paire */ 1302540 + 1098240 - 0,
        /* valets ou mieux    */ 0,   // recalculé ci-dessous
        /* deux paires        */ 123552,
        /* brelan             */ 54912,
        /* quinte             */ 10200,
        /* couleur            */ 5108,
        /* full               */ 3744,
        /* carré              */ 624,
        /* quinte flush       */ 36,
        /* quinte royale      */ 4,
    };
    // Une paire « valets ou mieux » : 4 rangs payants sur 13.
    const uint32_t pairsTotal = 1098240;
    const uint32_t jacksOrBetter = pairsTotal * 4 / 13;
    const uint32_t nothing = 1302540 + (pairsTotal - jacksOrBetter);

    uint32_t seen[core::kPokerRankCount] = {0};
    core::Hand h;
    for (int a = 0; a < 48; ++a)
      for (int b = a + 1; b < 49; ++b)
        for (int c = b + 1; c < 50; ++c)
          for (int d = c + 1; d < 51; ++d)
            for (int e = d + 1; e < 52; ++e) {
                core::handClear(h);
                const int idx[5] = {a, b, c, d, e};
                for (int i = 0; i < 5; ++i) {
                    core::handAdd(h, core::Card{
                        static_cast<uint8_t>(idx[i] % 13 + 1),
                        static_cast<uint8_t>(idx[i] / 13)});
                }
                seen[static_cast<uint8_t>(core::rankHand(h))]++;
            }

    uint32_t total = 0;
    for (uint8_t i = 0; i < core::kPokerRankCount; ++i) total += seen[i];
    std::printf("\n    %u mains enumerees\n", total);
    TEST_ASSERT_EQUAL_UINT32(2598960, total);

    TEST_ASSERT_EQUAL_UINT32_MESSAGE(4, seen[9], "quintes royales");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(36, seen[8], "quintes flush");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(624, seen[7], "carres");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(3744, seen[6], "fulls");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(5108, seen[5], "couleurs");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(10200, seen[4], "quintes");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(54912, seen[3], "brelans");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(123552, seen[2], "deux paires");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(jacksOrBetter, seen[1], "paires payantes");
    TEST_ASSERT_EQUAL_UINT32_MESSAGE(nothing, seen[0], "mains perdantes");
    (void)expected;

    // Retour d'une main tirée au hasard, sans aucun échange : c'est le
    // plancher du jeu, avant toute décision du joueur.
    double ev = 0;
    for (uint8_t i = 0; i < core::kPokerRankCount; ++i) {
        ev += static_cast<double>(seen[i]) *
              core::pokerPayout(static_cast<core::PokerRank>(i), false);
    }
    ev /= 2598960.0;
    std::printf("    retour sans echange = %.2f %%\n", ev * 100.0);
    TEST_ASSERT_TRUE(ev > 0.30 && ev < 0.45);
}

static void test_aces_work_at_both_ends_of_a_straight() {
    // A-2-3-4-5 : la quinte « du bas », que beaucoup d'implémentations
    // ratent parce qu'elles ne comptent l'As que haut.
    TEST_ASSERT_EQUAL(core::PokerRank::Straight, core::rankHand(mk({0, 1, 2, 3, 17})));
    // 10-V-D-R-A de couleurs mélangées.
    TEST_ASSERT_EQUAL(core::PokerRank::Straight, core::rankHand(mk({9, 10, 11, 12, 13})));
    // A-2-3-4-5 assortie : quinte flush, PAS royale.
    TEST_ASSERT_EQUAL(core::PokerRank::StraightFlush, core::rankHand(mk({0, 1, 2, 3, 4})));
    // 10-V-D-R-A assortie : royale.
    TEST_ASSERT_EQUAL(core::PokerRank::RoyalFlush, core::rankHand(mk({9, 10, 11, 12, 0})));
    // K-A-2-3-4 n'est PAS une quinte : l'As ne fait pas le tour.
    TEST_ASSERT_NOT_EQUAL(core::PokerRank::Straight, core::rankHand(mk({12, 0, 1, 2, 3})));
}

static void test_the_paying_threshold_is_jacks() {
    TEST_ASSERT_EQUAL(core::PokerRank::None,
                      core::rankHand(mk({8, 21, 0, 2, 4})));   // paire de 10
    TEST_ASSERT_EQUAL(core::PokerRank::JacksOrBetter,
                      core::rankHand(mk({10, 23, 0, 2, 4})));  // paire de valets
    TEST_ASSERT_EQUAL(core::PokerRank::JacksOrBetter,
                      core::rankHand(mk({0, 13, 3, 5, 7})));   // paire d'as
}

static void test_royal_flush_pays_more_at_max_bet() {
    // C'est le bonus qui donne une raison de miser gros — la signature du
    // video poker.
    TEST_ASSERT_EQUAL_UINT16(250, core::pokerPayout(core::PokerRank::RoyalFlush, false));
    TEST_ASSERT_EQUAL_UINT16(800, core::pokerPayout(core::PokerRank::RoyalFlush, true));
    // Aucun autre rang ne change avec la mise.
    for (uint8_t i = 0; i < core::kPokerRankCount - 1; ++i) {
        const core::PokerRank r = static_cast<core::PokerRank>(i);
        TEST_ASSERT_EQUAL_UINT16(core::pokerPayout(r, false), core::pokerPayout(r, true));
    }
    // Le barème 9/6 : c'est lui qui donne les 99,5 % du jeu bien joué.
    TEST_ASSERT_EQUAL_UINT16(9, core::pokerPayout(core::PokerRank::FullHouse, false));
    TEST_ASSERT_EQUAL_UINT16(6, core::pokerPayout(core::PokerRank::Flush, false));
}

static void test_payouts_increase_with_rank() {
    for (uint8_t i = 2; i < core::kPokerRankCount; ++i) {
        const uint16_t lo = core::pokerPayout(static_cast<core::PokerRank>(i - 1), false);
        const uint16_t hi = core::pokerPayout(static_cast<core::PokerRank>(i), false);
        TEST_ASSERT_TRUE_MESSAGE(hi > lo, "gains non croissants avec le rang");
    }
}

static void test_deck_is_a_single_deck_and_deals_uniquely() {
    // Un sabot de quatre jeux changerait le jeu en silence : quatre rois
    // possibles au lieu de quatre au total.
    core::seedXorShift(4242);
    core::Deck d;
    core::shuffleDeck(d, core::xorShift32);
    int count[52] = {0};
    for (int i = 0; i < 52; ++i) count[d.card[i]]++;
    for (int i = 0; i < 52; ++i) TEST_ASSERT_EQUAL_INT(1, count[i]);

    // Dix cartes tirées d'affilée sont toutes différentes — c'est ce dont
    // dépend l'échange de cinq cartes.
    bool seen[52] = {false};
    for (int i = 0; i < 10; ++i) {
        const core::Card c = core::dealFromDeck(d);
        const int idx = (c.suit * 13) + (c.rank - 1);
        TEST_ASSERT_FALSE_MESSAGE(seen[idx], "carte distribuee deux fois");
        seen[idx] = true;
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_every_possible_hand_is_classified);
    RUN_TEST(test_aces_work_at_both_ends_of_a_straight);
    RUN_TEST(test_the_paying_threshold_is_jacks);
    RUN_TEST(test_royal_flush_pays_more_at_max_bet);
    RUN_TEST(test_payouts_increase_with_rank);
    RUN_TEST(test_deck_is_a_single_deck_and_deals_uniquely);
    return UNITY_END();
}
