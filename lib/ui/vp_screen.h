#pragma once
#include <cstdint>

#include "hal_display.h"
#include "vp_session.h"

namespace ui {

void drawVpScreen(lgfx::LGFX_Sprite& g, const core::VpSession& s, uint32_t now);

}  // namespace ui
