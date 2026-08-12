#pragma once
#include <cstdint>

#include "boot.h"
#include "hal_display.h"

namespace ui {

// Dessine la séquence d'allumage. `elapsed` est le temps écoulé depuis le
// démarrage, en millisecondes.
void drawBootFx(lgfx::LGFX_Sprite& g, uint32_t elapsed);

}  // namespace ui
