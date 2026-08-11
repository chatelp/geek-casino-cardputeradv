#pragma once
#include <cstdint>
#include "hal_display.h"

namespace ui {

// Écran de démarrage / mire de validation du pipeline de rendu.
// Déterministe : même frame → mêmes pixels (requis pour --shot).
void drawBootScreen(lgfx::LGFX_Sprite& g, uint32_t frame);

}  // namespace ui
