// Blackjack : les règles se vérifient sans écran ni cartes en main.
#include <cstdio>
#include <initializer_list>
#include <unity.h>

#include "blackjack.h"
#include "cards.h"

void setUp() {}
void tearDown() {}

static core::Hand hand(std::initializer_list<uint8_t> ranks) {
    core::Hand h;
    core::handClear(h);
    for (const uint8_t r : ranks) core::handAdd(h, core::Card{r, 0});
    return h;
}

// ------------------------------------------------------------ valeur d'une main
static void test_aces_take_the_best_value() {
    // Tout le blackjack tient dans cette règle : l'As vaut 11 tant qu'il
    // peut, 1 ensuite.
    TEST_ASSERT_EQUAL_UINT8(21, core::handValue(hand({1, 13})).total);      // A+K
    TEST_ASSERT_TRUE(core::handValue(hand({1, 6})).soft);                   // A+6 = 17 souple
    TEST_ASSERT_EQUAL_UINT8(17, core::handValue(hand({1, 6})).total);
    // A+6+10 : l'As doit retomber à 1 → 17 dur, pas 27.
    const core::HandValue v = core::handValue(hand({1, 6, 10}));
    TEST_ASSERT_EQUAL_UINT8(17, v.total);
    TEST_ASSERT_FALSE(v.soft);
    // Deux As : un seul peut valoir 11.
    TEST_ASSERT_EQUAL_UINT8(12, core::handValue(hand({1, 1})).total);
    TEST_ASSERT_EQUAL_UINT8(13, core::handValue(hand({1, 1, 1})).total);
    // Les figures valent toutes 10.
    TEST_ASSERT_EQUAL_UINT8(30, core::handValue(hand({11, 12, 13})).total);
}

static void test_blackjack_is_exactly_two_cards() {
    TEST_ASSERT_TRUE(core::isBlackjack(hand({1, 12})));
    // 21 en trois cartes n'est PAS un blackjack : il paie 1:1, pas 3:2.
    TEST_ASSERT_FALSE(core::isBlackjack(hand({7, 7, 7})));
    TEST_ASSERT_TRUE(core::isBust(hand({10, 10, 5})));
    TEST_ASSERT_FALSE(core::isBust(hand({10, 10, 1})));  // l'As sauve la main
}

// ------------------------------------------------------------------- sabot
static void test_shoe_holds_every_card_the_right_number_of_times() {
    core::seedXorShift(1);
    core::Shoe s{};
    core::shuffleShoe(s, core::xorShift32);
    int count[52] = {0};
    for (uint16_t i = 0; i < core::kShoeSize; ++i) count[s.card[i]]++;
    for (int i = 0; i < 52; ++i) {
        TEST_ASSERT_EQUAL_INT_MESSAGE(core::kDecks, count[i],
                                      "carte absente ou en trop dans le sabot");
    }
}

static void test_shoe_reshuffles_instead_of_running_dry() {
    core::seedXorShift(2);
    core::Shoe s{};
    for (int i = 0; i < 5000; ++i) {
        const core::Card c = core::dealCard(s, core::xorShift32);
        TEST_ASSERT_TRUE(c.rank >= 1 && c.rank <= 13);
        TEST_ASSERT_TRUE(c.suit < 4);
    }
}

// ------------------------------------------------------------------ paiements
static void test_payouts_follow_the_rules() {
    // Blackjack 3:2 — mise 10 rendue 25 (10 + 15).
    TEST_ASSERT_EQUAL_UINT32(25, core::bjPayoutFor(core::BjOutcome::PlayerBlackjack, 10));
    // Mise impaire : l'arrondi va au joueur (5 → 5 + 8 = 13, pas 12).
    TEST_ASSERT_EQUAL_UINT32(13, core::bjPayoutFor(core::BjOutcome::PlayerBlackjack, 5));
    TEST_ASSERT_EQUAL_UINT32(20, core::bjPayoutFor(core::BjOutcome::PlayerWin, 10));
    TEST_ASSERT_EQUAL_UINT32(20, core::bjPayoutFor(core::BjOutcome::DealerBust, 10));
    TEST_ASSERT_EQUAL_UINT32(10, core::bjPayoutFor(core::BjOutcome::Push, 10));
    TEST_ASSERT_EQUAL_UINT32(0, core::bjPayoutFor(core::BjOutcome::DealerWin, 10));
    TEST_ASSERT_EQUAL_UINT32(0, core::bjPayoutFor(core::BjOutcome::PlayerBust, 10));
}

// ------------------------------------------------------------ règle du croupier
static void test_dealer_stands_on_all_seventeens() {
    core::seedXorShift(3);
    core::Blackjack b{};
    core::shuffleShoe(b.shoe, core::xorShift32);

    // 17 dur : il reste.
    b.dealer = hand({10, 7});
    b.phase = core::BjPhase::DealerTurn;
    TEST_ASSERT_FALSE(core::bjDealerStep(b, core::xorShift32));
    TEST_ASSERT_EQUAL_UINT8(2, b.dealer.n);

    // 17 souple (A+6) : il reste aussi — c'est la règle S17.
    b.dealer = hand({1, 6});
    b.phase = core::BjPhase::DealerTurn;
    TEST_ASSERT_FALSE(core::bjDealerStep(b, core::xorShift32));
    TEST_ASSERT_EQUAL_UINT8(2, b.dealer.n);

    // 16 : il tire.
    b.dealer = hand({10, 6});
    b.phase = core::BjPhase::DealerTurn;
    TEST_ASSERT_TRUE(core::bjDealerStep(b, core::xorShift32));
    TEST_ASSERT_EQUAL_UINT8(3, b.dealer.n);
}

// ------------------------------------------------------------------ arbitrage
static void test_settlement_compares_totals_correctly() {
    core::Economy e = core::freshEconomy();
    core::Blackjack b{};
    b.stake = 10;

    auto settle = [&](core::Hand p, core::Hand d) {
        b.player = p; b.dealer = d; b.outcome = core::BjOutcome::None;
        core::bjSettle(b, e);
        return b.outcome;
    };

    TEST_ASSERT_EQUAL(core::BjOutcome::PlayerWin, settle(hand({10, 9}), hand({10, 8})));
    TEST_ASSERT_EQUAL(core::BjOutcome::DealerWin, settle(hand({10, 7}), hand({10, 8})));
    TEST_ASSERT_EQUAL(core::BjOutcome::Push, settle(hand({10, 8}), hand({10, 8})));
    TEST_ASSERT_EQUAL(core::BjOutcome::DealerBust, settle(hand({10, 8}), hand({10, 8, 9})));
    TEST_ASSERT_EQUAL(core::BjOutcome::PlayerBlackjack, settle(hand({1, 13}), hand({10, 9})));
    // Deux blackjacks : égalité, pas victoire du joueur.
    TEST_ASSERT_EQUAL(core::BjOutcome::Push, settle(hand({1, 13}), hand({1, 12})));
    // Blackjack du croupier contre 21 en trois cartes : le croupier gagne.
    TEST_ASSERT_EQUAL(core::BjOutcome::DealerWin, settle(hand({7, 7, 7}), hand({1, 10})));
}

// ------------------------------------------------------------------- doublement
static void test_double_takes_one_card_and_doubles_the_stake() {
    core::seedXorShift(4);
    core::Economy e = core::freshEconomy();
    core::Blackjack b{};
    TEST_ASSERT_TRUE(core::bjDeal(b, e, core::xorShift32));
    if (b.phase != core::BjPhase::PlayerTurn) return;  // blackjack immédiat

    const uint16_t stake0 = b.stake;
    const int32_t credits0 = e.credits;
    TEST_ASSERT_TRUE(core::bjCanDouble(b, e));
    core::bjAct(b, core::BjAction::Double, e, core::xorShift32);

    TEST_ASSERT_EQUAL_UINT16(stake0 * 2, b.stake);   // mise doublée
    TEST_ASSERT_EQUAL_UINT8(3, b.player.n);          // exactement une carte
    TEST_ASSERT_FALSE(core::bjCanDouble(b, e));      // et pas deux fois
    // Le solde a bien été débité une seconde fois (au gain près, si le
    // doublement a fait sauter et déclenché le règlement).
    TEST_ASSERT_EQUAL_INT32(credits0 - stake0 + static_cast<int32_t>(b.payout),
                            e.credits);
}

// ---------------------------------------------------------------- session longue
static void test_long_session_stays_sane() {
    // 20 000 mains avec une stratégie simple (tirer sous 17). Vérifie qu'on
    // ne bloque jamais, que le solde reste positif, et donne le RTP obtenu.
    core::seedXorShift(0xB1ACC);
    core::Economy e = core::freshEconomy();
    core::Blackjack b{};
    uint64_t staked = 0, returned = 0;
    int hands = 0;

    for (int i = 0; i < 20000; ++i) {
        e.credits = 100000;  // isole la mesure de l'économie
        e.betIndex = core::kDefaultBetIndex;
        if (!core::bjDeal(b, e, core::xorShift32)) break;
        ++hands;

        int guard = 0;
        while (b.phase == core::BjPhase::PlayerTurn && ++guard < 20) {
            if (core::handValue(b.player).total < 17) {
                core::bjAct(b, core::BjAction::Hit, e, core::xorShift32);
            } else {
                core::bjAct(b, core::BjAction::Stand, e, core::xorShift32);
            }
        }
        TEST_ASSERT_TRUE_MESSAGE(guard < 20, "tour joueur sans fin");

        guard = 0;
        while (core::bjDealerStep(b, core::xorShift32) && ++guard < 20) {}
        TEST_ASSERT_TRUE_MESSAGE(guard < 20, "tour croupier sans fin");

        if (b.phase != core::BjPhase::Settle || b.outcome == core::BjOutcome::None) {
            core::bjSettle(b, e);
        }
        TEST_ASSERT_TRUE(e.credits >= 0);
        staked += b.stake;
        returned += b.payout;
    }

    TEST_ASSERT_TRUE_MESSAGE(hands > 19000, "trop peu de mains jouees");
    const double rtp = static_cast<double>(returned) / static_cast<double>(staked);
    std::printf("\n    %d mains, RTP = %.2f %% (strategie naive < 17)\n",
                hands, rtp * 100.0);
    // La stratégie naïve perd plus que la stratégie de base (~99,5 %), mais
    // le jeu ne doit pas être une arnaque pour autant.
    TEST_ASSERT_TRUE_MESSAGE(rtp > 0.85, "RTP anormalement bas : regles suspectes");
    TEST_ASSERT_TRUE_MESSAGE(rtp < 1.05, "RTP anormalement haut : regles suspectes");
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_aces_take_the_best_value);
    RUN_TEST(test_blackjack_is_exactly_two_cards);
    RUN_TEST(test_shoe_holds_every_card_the_right_number_of_times);
    RUN_TEST(test_shoe_reshuffles_instead_of_running_dry);
    RUN_TEST(test_payouts_follow_the_rules);
    RUN_TEST(test_dealer_stands_on_all_seventeens);
    RUN_TEST(test_settlement_compares_totals_correctly);
    RUN_TEST(test_double_takes_one_card_and_doubles_the_stake);
    RUN_TEST(test_long_session_stays_sane);
    return UNITY_END();
}
