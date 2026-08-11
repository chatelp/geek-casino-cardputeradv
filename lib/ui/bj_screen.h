#pragma once
#include <cstdint>

#include "bj_session.h"
#include "hal_display.h"

namespace ui {

void drawBjScreen(lgfx::LGFX_Sprite& g, const core::BjSession& s, uint32_t now);

// Une carte, réutilisée par la page d'aide.
void drawCard(lgfx::LGFX_Sprite& g, core::Card c, int x, int y, bool faceDown);

}  // namespace ui
