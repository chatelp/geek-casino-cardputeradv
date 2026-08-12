#include "cards.h"

namespace core {

namespace {
// Coupe : on remélange avant la fin du sabot plutôt que de le vider.
constexpr uint16_t kReshuffleAt = kShoeSize / 4;

uint8_t pipValue(uint8_t rank) {
    if (rank >= 10) return 10;  // figures
    return rank;                // l'As vaut 1 ici ; le +10 se fait dans handValue
}
}  // namespace

void handClear(Hand& h) { h.n = 0; }

void handAdd(Hand& h, Card card) {
    if (h.n < kHandMax) h.c[h.n++] = card;
}

HandValue handValue(const Hand& h) {
    uint8_t total = 0;
    bool hasAce = false;
    for (uint8_t i = 0; i < h.n; ++i) {
        total = static_cast<uint8_t>(total + pipValue(h.c[i].rank));
        if (h.c[i].rank == 1) hasAce = true;
    }
    // Un seul As peut valoir 11 : deux feraient 22.
    if (hasAce && total + 10 <= 21) {
        return {static_cast<uint8_t>(total + 10), true};
    }
    return {total, false};
}

bool isBlackjack(const Hand& h) {
    return h.n == 2 && handValue(h).total == 21;
}

bool isBust(const Hand& h) { return handValue(h).total > 21; }

void shuffleDeck(Deck& d, RngFn rng) {
    for (uint8_t i = 0; i < 52; ++i) d.card[i] = i;
    for (uint8_t i = 51; i > 0; --i) {
        const uint8_t j = static_cast<uint8_t>(drawBelow(rng, i + 1));
        const uint8_t t = d.card[i];
        d.card[i] = d.card[j];
        d.card[j] = t;
    }
    d.pos = 0;
}

Card dealFromDeck(Deck& d) {
    if (d.pos >= 52) d.pos = 0;  // ne devrait pas arriver : 10 cartes max
    const uint8_t idx = d.card[d.pos++];
    return Card{static_cast<uint8_t>(idx % 13 + 1), static_cast<uint8_t>(idx / 13)};
}

uint8_t deckLeft(const Deck& d) {
    return d.pos >= 52 ? 0 : static_cast<uint8_t>(52 - d.pos);
}

void shuffleShoe(Shoe& s, RngFn rng) {
    for (uint16_t i = 0; i < kShoeSize; ++i) {
        s.card[i] = static_cast<uint8_t>(i % 52);
    }
    // Fisher-Yates avec le tirage sans biais de modulo.
    for (uint16_t i = kShoeSize - 1; i > 0; --i) {
        const uint16_t j = static_cast<uint16_t>(drawBelow(rng, i + 1));
        const uint8_t t = s.card[i];
        s.card[i] = s.card[j];
        s.card[j] = t;
    }
    s.pos = 0;
    s.ready = true;
}

uint16_t cardsLeft(const Shoe& s) {
    return s.pos >= kShoeSize ? 0 : static_cast<uint16_t>(kShoeSize - s.pos);
}

bool needsShuffle(const Shoe& s) {
    return !s.ready || cardsLeft(s) <= kReshuffleAt;
}

Card dealCard(Shoe& s, RngFn rng) {
    if (!s.ready || s.pos >= kShoeSize) shuffleShoe(s, rng);
    const uint8_t idx = s.card[s.pos++];
    return Card{static_cast<uint8_t>(idx % 13 + 1), static_cast<uint8_t>(idx / 13)};
}

}  // namespace core
