// Bandes de rouleaux — logique pure, aucun include Arduino/M5/lgfx.
//
// C'est la BANDE qui fixe les probabilités, pas la table de gains : un
// symbole est d'autant plus fréquent qu'il occupe de positions. Table de
// gains et bande se lisent ensemble, jamais séparément.
#pragma once
#include <cstdint>

#include "rng.h"
#include "symbol_ids.h"

namespace core {

// Le MVP utilise 3 rouleaux ; le cœur est dimensionné pour aller à 5 sans
// refonte (décision D-003).
constexpr uint8_t kMaxReels = 5;
constexpr uint8_t kMvpReels = 3;
constexpr uint16_t kStripLen = 32;

struct ReelSet {
    uint8_t reels;
    const uint8_t* strip[kMaxReels];
    uint16_t len[kMaxReels];
};

// Le jeu de bandes du MVP : les trois rouleaux partagent la même bande.
const ReelSet& mvpReelSet();

// Symbole à une position, avec bouclage (la bande est un anneau).
uint8_t symbolAt(const ReelSet& rs, uint8_t reel, int32_t pos);

// Combien de fois `sym` apparaît sur la bande d'un rouleau.
uint16_t countOn(const ReelSet& rs, uint8_t reel, uint8_t sym);

// Tire une position par rouleau. `pos` et `sym` reçoivent rs.reels valeurs.
void spin(const ReelSet& rs, RngFn rng, uint16_t* pos, uint8_t* sym);

}  // namespace core
