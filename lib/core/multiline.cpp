#include "multiline.h"

namespace core {

namespace {

// Centre, haut, bas, chevron bas, chevron haut. Les deux derniers donnent
// leur relief au format : sans eux, trois lignes parallèles se lisent comme
// trois machines côte à côte.
constexpr Payline kLines[kVideoLines] = {
    {{1, 1, 1, 1, 1}},
    {{0, 0, 0, 0, 0}},
    {{2, 2, 2, 2, 2}},
    {{0, 1, 2, 1, 0}},
    {{2, 1, 0, 1, 2}},
};

// Gains par symbole et par longueur, calibrés pour un RTP par ligne de
// 94,95 % — vérifié analytiquement par test_multiline. Le 5 d'affilée est
// mythique (1 tour sur 210 000) ; c'est le 4 d'affilée, 1 sur 14 000, qui
// fait les vrais moments de la partie.
//                     0  1  2    3     4      5
constexpr MultiPaytable kPay = {{
    {0, 0, 0,     8,    20,    50},    // RESISTOR (7/32)
    {0, 0, 0,    15,    40,   100},    // LED      (6/32)
    {0, 0, 0,    25,    75,   200},    // CHIP     (5/32)
    {0, 0, 0,    30,   150,   600},    // FLOPPY   (4/32)
    {0, 0, 0,    75,   500,  2500},    // GAMEPAD  (3/32)
    {0, 0, 0,   100,   600,  3000},    // CRT      (3/32)
    {0, 0, 0,   200,  2000, 12500},    // D20      (2/32)
    {0, 0, 0,   250,  2500, 15000},    // INVADER  (2/32) — jackpot
}};

// Bande PROPRE au format vidéo. Sur cinq rouleaux, la bande du 3×1 rendrait
// le jackpot introuvable (1 tour sur 33 millions) : l'invader y gagne une
// seconde position. Effectifs : 7/6/5/4/3/3/2/2 = 32.
// Deux voisins ne sont jamais identiques, même invariant qu'au 3×1.
constexpr uint8_t kVideoStrip[kStripLen] = {
    SYM_RESISTOR, SYM_LED, SYM_RESISTOR, SYM_LED,
    SYM_RESISTOR, SYM_CHIP, SYM_RESISTOR, SYM_LED,
    SYM_CHIP, SYM_FLOPPY, SYM_RESISTOR, SYM_LED,
    SYM_CHIP, SYM_FLOPPY, SYM_GAMEPAD, SYM_CRT,
    SYM_RESISTOR, SYM_LED, SYM_CHIP, SYM_FLOPPY,
    SYM_GAMEPAD, SYM_CRT, SYM_D20, SYM_INVADER,
    SYM_RESISTOR, SYM_LED, SYM_CHIP, SYM_FLOPPY,
    SYM_GAMEPAD, SYM_CRT, SYM_D20, SYM_INVADER,
};

static_assert(kSymbolCount == 8,
              "La table vidéo est calibrée pour 8 symboles : en ajouter un "
              "impose de refaire l'équilibrage, pas d'allonger le tableau.");

constexpr ReelSet kVideoSet = {
    kVideoReels,
    {kVideoStrip, kVideoStrip, kVideoStrip, kVideoStrip, kVideoStrip},
    {kStripLen, kStripLen, kStripLen, kStripLen, kStripLen},
};

}  // namespace

const Payline* videoPaylines() { return kLines; }
const MultiPaytable& videoPaytable() { return kPay; }
const ReelSet& videoReelSet() { return kVideoSet; }

void fillGrid(const ReelSet& rs, GridOutcome& out, uint8_t rows) {
    for (uint8_t r = 0; r < rs.reels; ++r) {
        for (uint8_t y = 0; y < rows; ++y) {
            out.sym[r][y] = symbolAt(rs, r, static_cast<int32_t>(out.pos[r]) + y);
        }
    }
}

void spinGrid(const ReelSet& rs, RngFn rng, GridOutcome& out, uint8_t rows) {
    for (uint8_t r = 0; r < rs.reels; ++r) {
        out.pos[r] = static_cast<uint16_t>(drawBelow(rng, rs.len[r]));
    }
    fillGrid(rs, out, rows);
}

void evaluateGrid(const MultiPaytable& pt, const Payline* lines, uint8_t nLines,
                  uint8_t reels, GridOutcome& out) {
    out.winCount = 0;
    out.totalMultiplier = 0;
    out.jackpot = false;

    for (uint8_t l = 0; l < nLines; ++l) {
        const uint8_t first = out.sym[0][lines[l].row[0]];
        uint8_t run = 1;
        while (run < reels && out.sym[run][lines[l].row[run]] == first) ++run;

        const uint16_t m = pt.pay[first][run];
        if (m == 0) continue;

        out.wins[out.winCount++] = LineWin{l, first, run, m};
        out.totalMultiplier += m;
        if (first == kJackpotSymbol && run == reels) out.jackpot = true;
    }
}

namespace {

// Espérance d'une ligne, en multiplicateurs de la mise par ligne.
// P(exactement k alignés depuis la gauche) = p^k (1-p) pour k < reels,
// et p^reels pour la ligne pleine.
void lineStats(const ReelSet& rs, const MultiPaytable& pt, uint8_t reels,
               double& ev, double& hit) {
    ev = 0;
    hit = 0;
    const double n = static_cast<double>(rs.len[0]);
    for (uint8_t s = 0; s < kSymbolCount; ++s) {
        const double p = static_cast<double>(countOn(rs, 0, s)) / n;
        double pk = 1.0;
        for (uint8_t k = 1; k <= reels; ++k) {
            pk *= p;  // p^k
            const double prob = (k == reels) ? pk : pk * (1.0 - p);
            const uint16_t m = pt.pay[s][k];
            if (m > 0) {
                ev += prob * m;
                hit += prob;
            }
        }
    }
}

}  // namespace

double exactLineRtp(const ReelSet& rs, const MultiPaytable& pt, uint8_t reels) {
    double ev = 0, hit = 0;
    lineStats(rs, pt, reels, ev, hit);
    return ev;
}

double exactLineHitRate(const ReelSet& rs, const MultiPaytable& pt, uint8_t reels) {
    double ev = 0, hit = 0;
    lineStats(rs, pt, reels, ev, hit);
    return hit;
}

}  // namespace core
