// Grille et lignes de paiement du format vidéo 5×3.
//
// La table de gains n'est PLUS ici : elle est unique et vit dans
// paytable.h. Ce fichier ne garde que ce qui est propre au format —
// la grille, les lignes, et la bande dédiée.
#pragma once
#include <cstdint>

#include "paytable.h"
#include "reels.h"
#include "rng.h"
#include "symbol_ids.h"

namespace core {

constexpr uint8_t kMaxRows = 3;
constexpr uint8_t kMaxLines = 5;
constexpr uint8_t kVideoReels = 5;
constexpr uint8_t kVideoRows = 3;
constexpr uint8_t kVideoLines = 5;

// Une ligne de paiement désigne une rangée par rouleau.
struct Payline {
    uint8_t row[kMaxReels];
};

const Payline* videoPaylines();

// Bande PROPRE au format vidéo — le seul doublon volontaire du projet.
const ReelSet& videoReelSet();

struct GridOutcome {
    uint16_t pos[kMaxReels];
    uint8_t sym[kMaxReels][kMaxRows];
    LineWin wins[kMaxLines];
    uint8_t winCount;
    uint32_t totalMultiplier;
    bool jackpot;
};

void fillGrid(const ReelSet& rs, GridOutcome& out, uint8_t rows);
void spinGrid(const ReelSet& rs, RngFn rng, GridOutcome& out, uint8_t rows);

// Évalue chaque ligne via evaluateLine — la seule fonction de calcul de
// gain du projet.
void evaluateGrid(const Paytable& pt, const Payline* lines, uint8_t nLines,
                  uint8_t reels, GridOutcome& out);

}  // namespace core
