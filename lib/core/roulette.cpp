#include "roulette.h"

namespace core {

namespace {

// La séquence réelle d'une roue européenne, dans le sens horaire.
constexpr uint8_t kWheel[kPockets] = {
    0, 32, 15, 19, 4, 21, 2, 25, 17, 34, 6, 27, 13, 36, 11, 30, 8, 23,
    10, 5, 24, 16, 33, 1, 20, 14, 31, 9, 22, 18, 29, 7, 28, 12, 35, 3, 26,
};

// Les rouges, tels que définis sur le tapis — aucune règle arithmétique
// ne les donne, il faut la table.
constexpr uint8_t kReds[18] = {
    1, 3, 5, 7, 9, 12, 14, 16, 18, 19, 21, 23, 25, 27, 30, 32, 34, 36,
};

const char* kNames[kBetKinds] = {
    "RED", "BLACK", "ODD", "EVEN", "1-18", "19-36",
    "1ST 12", "2ND 12", "3RD 12", "STRAIGHT",
};

}  // namespace

uint8_t pocketAt(uint8_t index) { return kWheel[index % kPockets]; }

uint8_t indexOfPocket(uint8_t number) {
    for (uint8_t i = 0; i < kPockets; ++i) {
        if (kWheel[i] == number) return i;
    }
    return 0;
}

bool isRed(uint8_t number) {
    for (uint8_t i = 0; i < 18; ++i) {
        if (kReds[i] == number) return true;
    }
    return false;
}

const char* betName(BetKind k) {
    const uint8_t i = static_cast<uint8_t>(k);
    return i < kBetKinds ? kNames[i] : "";
}

uint16_t roulettePayout(BetKind k) {
    switch (k) {
        case BetKind::Dozen1:
        case BetKind::Dozen2:
        case BetKind::Dozen3: return 3;
        case BetKind::Straight: return 36;
        default: return 2;  // chances simples
    }
}

bool betWins(BetKind k, uint8_t straightPick, uint8_t n) {
    // Le zéro fait perdre TOUTES les chances simples et les douzaines :
    // c'est là, et uniquement là, que la maison prend son avantage.
    if (k == BetKind::Straight) return n == straightPick;
    if (n == 0) return false;
    switch (k) {
        case BetKind::Red: return isRed(n);
        case BetKind::Black: return isBlack(n);
        case BetKind::Odd: return (n & 1) != 0;
        case BetKind::Even: return (n & 1) == 0;
        case BetKind::Low: return n <= 18;
        case BetKind::High: return n >= 19;
        case BetKind::Dozen1: return n <= 12;
        case BetKind::Dozen2: return n >= 13 && n <= 24;
        case BetKind::Dozen3: return n >= 25;
        default: return false;
    }
}

double exactRouletteRtp(BetKind k, uint8_t straightPick) {
    uint8_t wins = 0;
    for (uint8_t n = 0; n < kPockets; ++n) {
        if (betWins(k, straightPick, n)) ++wins;
    }
    return static_cast<double>(wins) * roulettePayout(k) /
           static_cast<double>(kPockets);
}

}  // namespace core
