#include "paytable.h"

namespace core {

namespace {

// Choisie avec la bande de reels.cpp pour un RTP d'environ 95 %.
// Toute retouche ici DOIT être revalidée par test/test_paytable, qui
// recalcule le RTP exact et refuse de passer hors de la fourchette.
constexpr Paytable kMvp = {
    {
        8,     // RESISTOR — 8 positions sur 32
        12,    // LED      — 7
        20,    // CHIP     — 6
        50,    // FLOPPY   — 4
        100,   // GAMEPAD  — 3
        250,   // CRT      — 2
        400,   // D20      — 1
        1200,  // INVADER  — 1, le jackpot
    },
    2,  // deux identiques en tête
};

static_assert(kSymbolCount == 8,
              "La table de gains est calibrée pour 8 symboles. Si l'art en "
              "ajoute ou en retire, il faut refaire l'équilibrage et les "
              "tests de RTP — pas seulement rallonger ce tableau.");

}  // namespace

const Paytable& mvpPaytable() { return kMvp; }

WinResult evaluate(const Paytable& pt, const uint8_t* sym, uint8_t reels) {
    WinResult w = {0, 0, 0, false};
    if (reels < 2) return w;

    // Longueur de la séquence identique depuis la gauche.
    uint8_t run = 1;
    while (run < reels && sym[run] == sym[0]) ++run;

    if (run >= 3) {
        w.multiplier = pt.three[sym[0]];
        w.matched = 3;
        w.symbol = sym[0];
        w.jackpot = (sym[0] == kJackpotSymbol);
    } else if (run == 2) {
        w.multiplier = pt.two;
        w.matched = 2;
        w.symbol = sym[0];
    }
    return w;
}

namespace {

// Parcourt récursivement toutes les combinaisons de positions et accumule
// le multiplicateur total ainsi que le nombre de tours gagnants.
void walk(const ReelSet& rs, const Paytable& pt, uint8_t reel, uint8_t* sym,
          double& sumMult, double& hits, double& total) {
    if (reel == rs.reels) {
        const WinResult w = evaluate(pt, sym, rs.reels);
        sumMult += w.multiplier;
        if (w.multiplier > 0) hits += 1.0;
        total += 1.0;
        return;
    }
    for (uint16_t i = 0; i < rs.len[reel]; ++i) {
        sym[reel] = rs.strip[reel][i];
        walk(rs, pt, reel + 1, sym, sumMult, hits, total);
    }
}

}  // namespace

double exactRtp(const ReelSet& rs, const Paytable& pt) {
    uint8_t sym[kMaxReels] = {0};
    double sumMult = 0, hits = 0, total = 0;
    walk(rs, pt, 0, sym, sumMult, hits, total);
    return total > 0 ? sumMult / total : 0.0;
}

double exactHitRate(const ReelSet& rs, const Paytable& pt) {
    uint8_t sym[kMaxReels] = {0};
    double sumMult = 0, hits = 0, total = 0;
    walk(rs, pt, 0, sym, sumMult, hits, total);
    return total > 0 ? hits / total : 0.0;
}

}  // namespace core
