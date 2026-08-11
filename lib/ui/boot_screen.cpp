#include "boot_screen.h"

namespace ui {

// PIÈGE lgfx : les couleurs doivent être typées uint16_t pour prendre le
// chemin RGB565. Un littéral nu (uint32) est interprété RGB888.
namespace {
constexpr uint16_t kBg      = 0x18E3;  // gris très sombre
constexpr uint16_t kBorder  = 0x4208;
constexpr uint16_t kTitle   = 0xFFFF;
constexpr uint16_t kSubtle  = 0xAD55;
constexpr uint16_t kDot     = 0xFD20;  // orange
}  // namespace

void drawBootScreen(lgfx::LGFX_Sprite& g, uint32_t frame) {
    const int w = g.width();
    const int h = g.height();

    g.fillScreen(kBg);
    g.drawRect(2, 2, w - 4, h - 4, kBorder);

    g.setTextDatum(lgfx::middle_center);
    g.setFont(&fonts::DejaVu24);
    g.setTextColor(kTitle, kBg);
    g.drawString("GEEK CASINO", w / 2, 46);

    g.setFont(&fonts::DejaVu12);
    g.setTextColor(kSubtle, kBg);
    g.drawString("bootstrap", w / 2, 72);

    // Mire de couleurs : valide profondeur, ordre des canaux et captures.
    const uint16_t swatches[] = {0xF800, 0x07E0, 0x001F, 0xFFE0, 0xF81F, 0x07FF, 0xFFFF};
    const int n = sizeof(swatches) / sizeof(swatches[0]);
    const int sw = 18, sh = 12;
    int x0 = (w - n * (sw + 4) + 4) / 2;
    for (int i = 0; i < n; ++i) {
        g.fillRect(x0 + i * (sw + 4), 96, sw, sh, swatches[i]);
    }

    // Témoin d'animation : la boucle tourne si le point se déplace.
    const int cx = 12 + static_cast<int>(frame % static_cast<uint32_t>(w - 24));
    g.fillCircle(cx, h - 10, 2, kDot);
}

}  // namespace ui
