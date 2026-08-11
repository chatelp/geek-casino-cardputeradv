#include "celebration.h"

#include "layout.h"
#include "painter.h"
#include "palette.h"
#include "symbols.h"

namespace ui {

using layout::kScreenH;
using layout::kScreenW;
namespace P = pal;

namespace {

// Le panneau arrive en s'ouvrant verticalement, sur les 12 % premiers de
// la célébration : assez pour qu'on voie le mouvement, assez court pour ne
// pas retarder la lecture du montant.
constexpr float kOpenFraction = 0.12f;

// Laissé volontairement plus étroit que l'écran : les rayons et les
// étincelles ont besoin de leur marge pour exister.
constexpr int kPanelW = 168;
constexpr int kPanelH = 58;

uint16_t tierColor(core::Tier t, bool demo) {
    if (demo) return kGrayLight;
    switch (t) {
        case core::Tier::Small: return P::cyan;
        case core::Tier::Mid: return P::yellow;
        case core::Tier::Big: return P::orange;
        case core::Tier::Jackpot: return P::green;
        default: return P::cyan;
    }
}

// Nombre d'étincelles par palier — l'escalade se joue là autant que dans
// la durée (D-008 : la sobriété du quotidien rend le jackpot énorme).
uint8_t sparkCount(core::Tier t) {
    switch (t) {
        case core::Tier::Small: return 0;
        case core::Tier::Mid: return 6;
        case core::Tier::Big: return 12;
        case core::Tier::Jackpot: return 18;
        default: return 0;
    }
}

bool pulse(uint32_t now, uint32_t periodMs) {
    return ((now / (periodMs / 2)) & 1u) != 0;
}

// Rayons partant des BORDS du panneau vers l'extérieur. Première version
// : ils rayonnaient du centre — donc entièrement cachés derrière le
// panneau, qui ne laisse que 36 px de marge. Depuis les bords, ils
// occupent la seule place réellement disponible.
void drawEdgeRays(lgfx::LGFX_Sprite& g, int px, int py, int pw, int ph,
                  uint32_t now, uint16_t col) {
    const uint32_t step = now / 110;
    // Côtés gauche et droit : les marges les plus larges.
    for (int i = 0; i < 5; ++i) {
        const int y = py + 8 + i * (ph - 16) / 4;
        const int len = 8 + static_cast<int>((step + i) % 4) * 6;
        for (int t = 3; t < len; t += 3) {
            if (px - t - 2 >= 0) g.fillRect(px - t - 2, y, 2, 2, col);
            if (px + pw + t < kScreenW) g.fillRect(px + pw + t, y, 2, 2, col);
        }
    }
    // Haut et bas : marges plus courtes, rayons plus courts.
    for (int i = 0; i < 7; ++i) {
        const int x = px + 10 + i * (pw - 20) / 6;
        const int len = 6 + static_cast<int>((step + i * 2) % 3) * 5;
        for (int t = 3; t < len; t += 3) {
            if (py - t - 2 >= 0) g.fillRect(x, py - t - 2, 2, 2, col);
            if (py + ph + t < kScreenH) g.fillRect(x, py + ph + t, 2, 2, col);
        }
    }
}

// Étincelles en orbite, projetées vers l'extérieur avec le temps. Positions
// déterministes : une capture reste reproductible.
void drawSparks(lgfx::LGFX_Sprite& g, const Celebration& c, uint32_t now,
                int cx, int cy) {
    const uint8_t n = sparkCount(c.tier);
    if (n == 0) return;
    static const int8_t kOrbit[18][2] = {
        {96, 20}, {-96, 24}, {80, -26}, {-84, -22}, {60, 30}, {-64, 32},
        {104, -8}, {-104, 6}, {40, -34}, {-44, -32}, {88, 34}, {-90, -34},
        {24, 36}, {-28, -36}, {110, 16}, {-110, -14}, {70, -36}, {-72, 36},
    };
    const uint16_t cols[3] = {P::yellow, P::orange, P::white};
    const uint32_t step = now / 100;
    for (uint8_t i = 0; i < n; ++i) {
        // Chaque étincelle s'éloigne puis revient : un cycle par seconde.
        const int phase = static_cast<int>((step + i * 3) % 10);
        const int reach = 70 + phase * 4;
        const int x = cx + kOrbit[i][0] * reach / 100;
        const int y = cy + kOrbit[i][1] * reach / 100;
        if (x < 2 || x > kScreenW - 4 || y < 2 || y > kScreenH - 4) continue;
        const int size = (i % 3 == 0) ? 3 : 2;
        g.fillRect(x, y, size, size,
                   c.demo ? kGrayMid : cols[(i + step) % 3]);
    }
}

}  // namespace

void celebrationShake(const Celebration& c, uint32_t now, int& dx, int& dy) {
    dx = 0;
    dy = 0;
    if (c.tier < core::Tier::Big || c.progress > 0.45f) return;
    // Deux pixels au plus, et seulement au début : un écran qui tremble
    // longtemps devient illisible au lieu d'être spectaculaire.
    const uint32_t k = now / 50;
    dx = static_cast<int>(k % 3) - 1;
    dy = static_cast<int>((k / 3) % 3) - 1;
}

void drawCelebration(lgfx::LGFX_Sprite& g, const Celebration& c, uint32_t now) {
    if (c.tier == core::Tier::None) return;

    const uint16_t accent = tierColor(c.tier, c.demo);
    const int cx = kScreenW / 2;
    const int cy = kScreenH / 2;

    // Ouverture : le panneau se déplie verticalement depuis son axe.
    float open = c.progress / kOpenFraction;
    if (open > 1.0f) open = 1.0f;
    const int h = static_cast<int>(kPanelH * open);
    if (h < 6) return;
    const int x = cx - kPanelW / 2;
    const int y = cy - h / 2;

    drawSparks(g, c, now, cx, cy);

    // Secousse : le panneau encaisse le coup, seulement sur les gros gains
    // et seulement à l'arrivée.
    int sx = 0, sy = 0;
    celebrationShake(c, now, sx, sy);
    const int px = x + sx, py = y + sy;

    if (c.tier >= core::Tier::Mid) {
        drawEdgeRays(g, px, py, kPanelW, h, now, c.demo ? kGrayMid : accent);
    }
    g.fillRect(px, py, kPanelW, h, P::ink900);
    drawFrame(g, px, py, kPanelW, h, accent, 2);
    // Liseré intérieur qui bat : le panneau respire sans clignoter.
    if (pulse(now, 300)) {
        drawFrame(g, px + 3, py + 3, kPanelW - 6, h - 6,
                  c.demo ? kGrayMid : P::white, 1);
    }
    if (h < kPanelH) return;  // encore en train de s'ouvrir

    if (c.tier == core::Tier::Jackpot) {
        drawText(g, "JACKPOT", px + kPanelW / 2, py + 7, accent, 2, Align::Center);
        // Trois invaders sous le mot : le jackpot est leur affaire.
        for (int i = 0; i < 3; ++i) {
            const int ix = px + 12 + i * 22;
            if (c.demo) drawSymbolGray(g, core::SYM_INVADER, ix, py + 24, 1);
            else drawSymbol(g, core::SYM_INVADER, ix, py + 24, 1);
        }
        drawNumber(g, static_cast<int32_t>(c.counted), px + kPanelW - 10, py + 26,
                   c.demo ? kGrayLight : P::white, 2, Align::Right);
        return;
    }

    drawText(g, "WIN", px + 8, py + 6, c.demo ? kGrayMid : P::steel300, 1);
    if (c.multiplier > 0 && c.tier >= core::Tier::Mid) {
        // Pastille du multiplicateur : dit POURQUOI le gain est gros.
        const int bw = numberWidth(c.multiplier, 1) + 12;
        g.fillRect(px + kPanelW - 6 - bw, py + 5, bw, 11, accent);
        drawText(g, "x", px + kPanelW - 3 - bw, py + 7, P::ink900, 1);
        drawNumber(g, c.multiplier, px + kPanelW - 10, py + 7, P::ink900, 1,
                   Align::Right);
    }

    // Le nombre qui grimpe — le vrai cœur de l'effet.
    drawNumber(g, static_cast<int32_t>(c.counted), px + kPanelW / 2, py + 21,
               c.demo ? kGrayLight : P::white, 3, Align::Center);
}

}  // namespace ui
