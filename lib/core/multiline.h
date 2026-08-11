// Machine à sous multi-lignes (5 rouleaux × 3 rangées) — logique pure.
//
// Module séparé du 3×1 volontairement : celui-ci est flashé et joué, on ne
// le refactorise pas sous les pieds de Pierre pour ajouter un format. Les
// deux partagent les bandes (reels.h) et le vocabulaire de symboles ; la
// fusion des deux tables de gains est une dette assumée, notée dans
// docs/DECISIONS.md.
#pragma once
#include <cstdint>

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

// Les 5 lignes du format vidéo : centre, haut, bas, puis les deux
// chevrons. L'ordre compte — la ligne 0 est celle qu'on affiche seule
// quand il faut expliquer le principe.
const Payline* videoPaylines();

// Multiplicateur par symbole ET par nombre d'alignés depuis la gauche.
// L'index EST le nombre de symboles : pay[s][3] = trois alignés.
struct MultiPaytable {
    uint16_t pay[kSymbolCount][kMaxReels + 1];
};

const MultiPaytable& videoPaytable();

// Jeu de bandes du format vidéo : 5 rouleaux, la même bande de 32.
const ReelSet& videoReelSet();

struct LineWin {
    uint8_t line;
    uint8_t symbol;
    uint8_t count;
    uint16_t multiplier;
};

struct GridOutcome {
    uint16_t pos[kMaxReels];                 // position de bande par rouleau
    uint8_t sym[kMaxReels][kMaxRows];        // grille visible
    LineWin wins[kMaxLines];
    uint8_t winCount;
    uint32_t totalMultiplier;                // somme des lignes
    bool jackpot;                            // 5 invaders sur une ligne
};

// Remplit la grille depuis les positions tirées. La rangée r d'un rouleau
// montre le symbole à (pos + r) : c'est la même bande vue par une fenêtre
// de trois.
void fillGrid(const ReelSet& rs, GridOutcome& out, uint8_t rows);

// Tire une grille complète.
void spinGrid(const ReelSet& rs, RngFn rng, GridOutcome& out, uint8_t rows);

// Évalue toutes les lignes. Lecture depuis la gauche, comme partout.
void evaluateGrid(const MultiPaytable& pt, const Payline* lines, uint8_t nLines,
                  uint8_t reels, GridOutcome& out);

// RTP exact PAR LIGNE, calculé analytiquement plutôt que par énumération :
// 32^5 = 33 millions de combinaisons, mais l'espérance d'une ligne ne
// dépend que des effectifs de la bande. Le total vaut nLignes × ce chiffre
// quand la mise est par ligne.
double exactLineRtp(const ReelSet& rs, const MultiPaytable& pt, uint8_t reels);
double exactLineHitRate(const ReelSet& rs, const MultiPaytable& pt, uint8_t reels);

}  // namespace core
