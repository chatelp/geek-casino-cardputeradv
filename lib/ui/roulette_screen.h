#pragma once
#include <cstdint>

#include "hal_display.h"
#include "roulette_session.h"

namespace ui {

void drawRouletteScreen(lgfx::LGFX_Sprite& g, const core::RouletteSession& s,
                        uint32_t now);

}  // namespace ui
