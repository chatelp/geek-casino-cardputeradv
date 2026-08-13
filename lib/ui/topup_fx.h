// La pluie de jetons du rachat — un overlay, par-dessus n'importe quel
// écran : le geste est transversal, son animation aussi.
#pragma once
#include "app.h"
#include "hal_display.h"

namespace ui {

void drawTopupFx(lgfx::LGFX_Sprite& g, const core::Topup& t, uint32_t now);

}  // namespace ui
