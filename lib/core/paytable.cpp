#include "paytable.h"

namespace core {

namespace {

// --- 3 rouleaux, 1 ligne. RTP exact 95,24 %, vérifié par test_paytable.
// Deux identiques en tête paient 2 quel que soit le symbole : le petit gain
// entretient le rythme, il ne récompense personne en particulier.
//                          0  1  2    3
constexpr Paytable kMvp = {{
    {0, 0, 2,    8, 0, 0},   // RESISTOR (8/32)
    {0, 0, 2,   12, 0, 0},   // LED      (7/32)
    {0, 0, 2,   20, 0, 0},   // CHIP     (6/32)
    {0, 0, 2,   50, 0, 0},   // FLOPPY   (4/32)
    {0, 0, 2,  100, 0, 0},   // GAMEPAD  (3/32)
    {0, 0, 2,  250, 0, 0},   // CRT      (2/32)
    {0, 0, 2,  400, 0, 0},   // D20      (1/32)
    {0, 0, 2, 1200, 0, 0},   // INVADER  (1/32) — jackpot
}};

// --- 5 rouleaux, 5 lignes. RTP par ligne 94,95 %, même vérification.
// Rien ne paie sous trois alignés : sur cinq rouleaux, une paire tomberait
// bien trop souvent.
//                            0  1  2    3     4      5
constexpr Paytable kVideo = {{
    {0, 0, 0,     8,    20,    50},    // RESISTOR (7/32)
    {0, 0, 0,    15,    40,   100},    // LED      (6/32)
    {0, 0, 0,    25,    75,   200},    // CHIP     (5/32)
    {0, 0, 0,    30,   150,   600},    // FLOPPY   (4/32)
    {0, 0, 0,    75,   500,  2500},    // GAMEPAD  (3/32)
    {0, 0, 0,   100,   600,  3000},    // CRT      (3/32)
    {0, 0, 0,   200,  2000, 12500},    // D20      (2/32)
    {0, 0, 0,   250,  2500, 15000},    // INVADER  (2/32) — jackpot
}};

static_assert(kSymbolCount == 8,
              "Les deux calibrages sont faits pour 8 symboles. En ajouter un "
              "impose de refaire l'équilibrage ET les tests de RTP, pas "
              "d'allonger les tableaux.");

}  // namespace

const Paytable& mvpPaytable() { return kMvp; }
const Paytable& videoPaytable() { return kVideo; }

LineWin evaluateLine(const Paytable& pt, const uint8_t* sym, uint8_t reels,
                     uint8_t line) {
    LineWin w = {line, 0, 0, 0, false};
    if (reels < 2) return w;

    uint8_t run = 1;
    while (run < reels && sym[run] == sym[0]) ++run;

    const uint16_t m = pt.pay[sym[0]][run];
    if (m == 0) return w;

    w.symbol = sym[0];
    w.count = run;
    w.multiplier = m;
    w.jackpot = (run == reels && sym[0] == kJackpotSymbol);
    return w;
}

namespace {

// P(exactement k alignés depuis la gauche) = (produit des p des k premiers
// rouleaux) × (1 - p du rouleau suivant), et sans le second facteur quand
// la ligne est pleine. Gère des bandes différentes d'un rouleau à l'autre.
void lineStats(const ReelSet& rs, const Paytable& pt, uint8_t reels,
               double& ev, double& hit) {
    ev = 0;
    hit = 0;
    for (uint8_t s = 0; s < kSymbolCount; ++s) {
        double prefix = 1.0;
        for (uint8_t k = 1; k <= reels; ++k) {
            const uint8_t r = static_cast<uint8_t>(k - 1);
            prefix *= static_cast<double>(countOn(rs, r, s)) /
                      static_cast<double>(rs.len[r]);
            double prob = prefix;
            if (k < reels) {
                const double pNext = static_cast<double>(countOn(rs, k, s)) /
                                     static_cast<double>(rs.len[k]);
                prob *= (1.0 - pNext);
            }
            const uint16_t m = pt.pay[s][k];
            if (m > 0) {
                ev += prob * m;
                hit += prob;
            }
        }
    }
}

}  // namespace

double exactLineRtp(const ReelSet& rs, const Paytable& pt, uint8_t reels) {
    double ev = 0, hit = 0;
    lineStats(rs, pt, reels, ev, hit);
    return ev;
}

double exactLineHitRate(const ReelSet& rs, const Paytable& pt, uint8_t reels) {
    double ev = 0, hit = 0;
    lineStats(rs, pt, reels, ev, hit);
    return hit;
}

}  // namespace core
