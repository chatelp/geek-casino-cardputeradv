// Rendu de l'écran de jeu. Ne décide rien : il affiche l'état que
// core::Game a calculé. Tout passe par un sprite plein écran, poussé d'un
// bloc par l'appelant.
#pragma once
#include <cstdint>

#include "game.h"
#include "hal_display.h"

namespace ui {

void drawSlotScreen(lgfx::LGFX_Sprite& g, const core::Game& game, uint32_t now,
                    bool classic = false);

}  // namespace ui
