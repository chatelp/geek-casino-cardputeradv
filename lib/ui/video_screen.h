#pragma once
#include <cstdint>

#include "hal_display.h"
#include "video_game.h"

namespace ui {

// Format vidéo 5×3 : aucun cabinet, aucun hublot. HUD en surimpression,
// lignes signalées par des chevrons latéraux.
void drawVideoScreen(lgfx::LGFX_Sprite& g, const core::VideoGame& game,
                     uint32_t now, bool classic);

}  // namespace ui
