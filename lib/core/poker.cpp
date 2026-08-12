#include "poker.h"

namespace core {

namespace {

constexpr uint16_t kPay[kPokerRankCount] = {
    0,     // None — sous la paire de valets
    1,     // JacksOrBetter
    2,     // TwoPair
    3,     // ThreeOfAKind
    4,     // Straight
    6,     // Flush        ← le « 6 » de 9/6
    9,     // FullHouse    ← le « 9 »
    25,    // FourOfAKind
    50,    // StraightFlush
    250,   // RoyalFlush
};
constexpr uint16_t kRoyalMaxBet = 800;

const char* kNames[kPokerRankCount] = {
    "", "JACKS OR BETTER", "TWO PAIR", "THREE OF A KIND", "STRAIGHT",
    "FLUSH", "FULL HOUSE", "FOUR OF A KIND", "STRAIGHT FLUSH", "ROYAL FLUSH",
};

}  // namespace

PokerRank rankHand(const Hand& h) {
    if (h.n != kPokerHandSize) return PokerRank::None;

    uint8_t mult[14] = {0};      // effectifs par rang, 1 = As
    bool present[15] = {false};  // 14 = As compté haut
    bool flush = true;

    for (uint8_t i = 0; i < kPokerHandSize; ++i) {
        const uint8_t r = h.c[i].rank;
        ++mult[r];
        present[r] = true;
        if (r == 1) present[14] = true;  // l'As sert aux deux quintes
        if (h.c[i].suit != h.c[0].suit) flush = false;
    }

    // Quinte : cinq rangs consécutifs. La borne 10 couvre 10-V-D-R-A.
    bool straight = false;
    uint8_t straightLow = 0;
    for (uint8_t s = 1; s <= 10; ++s) {
        if (present[s] && present[s + 1] && present[s + 2] &&
            present[s + 3] && present[s + 4]) {
            straight = true;
            straightLow = s;
            break;
        }
    }

    uint8_t pairs = 0, trips = 0, quads = 0, pairRank = 0;
    for (uint8_t r = 1; r <= 13; ++r) {
        if (mult[r] == 2) { ++pairs; pairRank = r; }
        else if (mult[r] == 3) ++trips;
        else if (mult[r] == 4) ++quads;
    }

    if (straight && flush) {
        return straightLow == 10 ? PokerRank::RoyalFlush : PokerRank::StraightFlush;
    }
    if (quads) return PokerRank::FourOfAKind;
    if (trips && pairs) return PokerRank::FullHouse;
    if (flush) return PokerRank::Flush;
    if (straight) return PokerRank::Straight;
    if (trips) return PokerRank::ThreeOfAKind;
    if (pairs == 2) return PokerRank::TwoPair;
    if (pairs == 1) {
        // Le seuil du jeu : une paire ne paie qu'à partir des valets.
        const bool high = pairRank == 1 || pairRank >= 11;
        return high ? PokerRank::JacksOrBetter : PokerRank::None;
    }
    return PokerRank::None;
}

uint16_t pokerPayout(PokerRank r, bool maxBet) {
    const uint8_t i = static_cast<uint8_t>(r);
    if (i >= kPokerRankCount) return 0;
    if (r == PokerRank::RoyalFlush && maxBet) return kRoyalMaxBet;
    return kPay[i];
}

const char* pokerRankName(PokerRank r) {
    const uint8_t i = static_cast<uint8_t>(r);
    return i < kPokerRankCount ? kNames[i] : "";
}

}  // namespace core
