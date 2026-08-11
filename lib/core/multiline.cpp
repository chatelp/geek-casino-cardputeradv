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

// Bande PROPRE au format vidéo — le seul doublon volontaire du projet.
// Sur cinq rouleaux, la bande du 3×1 rendrait le jackpot introuvable
// (1 tour sur 33 millions) : l'invader y gagne une seconde position.
// Effectifs : 7/6/5/4/3/3/2/2 = 32. Deux voisins ne sont jamais
// identiques, même invariant qu'au 3×1.
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

constexpr ReelSet kVideoSet = {
    kVideoReels,
    {kVideoStrip, kVideoStrip, kVideoStrip, kVideoStrip, kVideoStrip},
    {kStripLen, kStripLen, kStripLen, kStripLen, kStripLen},
};

}  // namespace

const Payline* videoPaylines() { return kLines; }
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

void evaluateGrid(const Paytable& pt, const Payline* lines, uint8_t nLines,
                  uint8_t reels, GridOutcome& out) {
    out.winCount = 0;
    out.totalMultiplier = 0;
    out.jackpot = false;

    for (uint8_t l = 0; l < nLines; ++l) {
        // On recompose la ligne, puis on la confie à l'évaluateur commun.
        uint8_t line[kMaxReels];
        for (uint8_t r = 0; r < reels; ++r) line[r] = out.sym[r][lines[l].row[r]];

        const LineWin w = evaluateLine(pt, line, reels, l);
        if (w.multiplier == 0) continue;

        out.wins[out.winCount++] = w;
        out.totalMultiplier += w.multiplier;
        if (w.jackpot) out.jackpot = true;
    }
}

}  // namespace core
