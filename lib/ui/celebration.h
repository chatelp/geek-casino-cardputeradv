// Célébration de gain, en surimpression. UNE implémentation pour les deux
// machines : la duplication d'un effet visuel produit exactement la même
// dérive silencieuse qu'une double table de gains.
#pragma once
#include <cstdint>

#include "game.h"
#include "hal_display.h"

namespace ui {

struct Celebration {
    core::Tier tier = core::Tier::None;
    uint32_t payout = 0;
    uint16_t multiplier = 0;   // affiché en pastille dès le palier moyen
    float progress = 0.0f;     // 0 à 1
    uint32_t counted = 0;      // montant affiché pendant le décompte
    bool demo = false;
};

// Décalage de tremblement à appliquer au reste de l'écran. Renvoie 0 hors
// des gros gains — l'écran ne bouge que quand ça vaut le coup.
void celebrationShake(const Celebration& c, uint32_t now, int& dx, int& dy);

// Dessine le panneau par-dessus le jeu. Ne dessine rien au palier None.
void drawCelebration(lgfx::LGFX_Sprite& g, const Celebration& c, uint32_t now);

}  // namespace ui
