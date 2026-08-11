#include "video_screen.h"

#include <cmath>

#include "layout.h"
#include "painter.h"
#include "palette.h"

namespace ui {

using namespace vlayout;
using layout::kScreenW;
using layout::kScreenH;
namespace P = pal;

namespace {

bool g_demo = false;
uint16_t A(uint16_t accent) { return g_demo ? kGrayLight : accent; }

bool blink(uint32_t now, uint32_t periodMs) {
    return ((now / (periodMs / 2)) & 1u) != 0;
}

// Traînées de glyphe : même principe qu'au 3×1, adapté à la cellule de 32.
constexpr float kBlurThreshold = 1.2f;
constexpr float kBlurSlowdown = 0.22f;

void drawBlurredColumn(lgfx::LGFX_Sprite& g, const core::ReelSet& rs, uint8_t c,
                       float pos, bool classic, bool gray) {
    const float apparent = pos * kBlurSlowdown;
    const float base = std::floor(apparent);
    const int shift = static_cast<int>((apparent - base) * kCell);
    const int streakH = kCell / 4;
    for (int k = -1; k <= kRows + 1; ++k) {
        const uint8_t sym = core::symbolAt(rs, c, static_cast<int32_t>(base) + k);
        const uint8_t* art = classic ? kSymbolsClassic[sym] : kSymbols[sym];
        const int top = cellY(0) + k * kCell - shift;
        for (int band = 0; band < 4; ++band) {
            const int row = 2 + band * 4;
            const int y = top + band * streakH;
            int x = 0;
            while (x < kSymbolPx) {
                const uint8_t idx = art[row * kSymbolPx + x];
                if (idx == 0) { ++x; continue; }
                int run = 1;
                while (x + run < kSymbolPx && art[row * kSymbolPx + x + run] == idx) ++run;
                g.fillRect(cellX(c) + x * kScale, y, run * kScale, streakH,
                           (gray ? kSymbolPaletteGray : kSymbolPalette)[idx]);
                x += run;
            }
        }
    }
}

// HUD en surimpression : deux chiffres posés sur le fond, sans bandeau.
void drawOverlayHud(lgfx::LGFX_Sprite& g, const core::VideoGame& game) {
    const core::Economy& e = game.econ;
    drawIcon(g, ICON_COIN, 3, 3);
    drawNumber(g, e.credits, 19, 3, A(P::yellow), 2);
    // La mise est PAR LIGNE : on affiche le total engagé à côté, sinon le
    // joueur croit miser cinq fois moins qu'il ne mise.
    const int32_t total = static_cast<int32_t>(core::videoStake(e));
    const int bw = numberWidth(total, 2);
    drawNumber(g, total, kScreenW - 8, 3, A(P::cyan), 2, Align::Right);
    drawText(g, "BET", kScreenW - 14 - bw, 3, P::steel300, 2, Align::Right);
    const bool idle = game.phase != core::Phase::Spinning && !g_demo;
    drawBetArrows(g, kScreenW - 14 - bw - 10, kScreenW - 6, 4, P::steel500,
                  idle && e.betIndex > 0,
                  idle && e.betIndex + 1 < core::kBetSteps);
}

void drawChevrons(lgfx::LGFX_Sprite& g, const core::VideoGame& game, uint32_t now,
                  uint8_t hot) {
    for (uint8_t l = 0; l < core::kVideoLines; ++l) {
        const uint8_t rowL = game.lines[l].row[0];
        const uint8_t rowR = game.lines[l].row[core::kVideoReels - 1];
        const bool on = (l == hot);
        const uint16_t col = on ? A(P::magenta) : P::ink600;
        const int yl = cellY(rowL) + kCell / 2;
        const int yr = cellY(rowR) + kCell / 2;
        for (int k = 0; k < 3; ++k) {
            g.fillRect(kGridX - 4 - k, yl - k, 1, 1 + 2 * k, col);
            g.fillRect(kGridX + kGridW + 3 + k, yr - k, 1, 1 + 2 * k, col);
        }
    }
}

// La ligne gagnante se trace en reliant les cellules — c'est la seule
// façon de rendre un chevron lisible sur une grille.
void drawWinPath(lgfx::LGFX_Sprite& g, const core::VideoGame& game, uint8_t hot,
                 uint32_t now) {
    if (hot >= core::kVideoLines || !blink(now, 340)) return;
    const core::Payline& pl = game.lines[hot];
    for (uint8_t c = 0; c < core::kVideoReels; ++c) {
        const int x = cellX(c), y = cellY(pl.row[c]);
        drawFrame(g, x - 1, y - 1, kCell + 2, kCell + 2, A(P::white), 1);
        if (c + 1 < core::kVideoReels) {
            const int y2 = cellY(pl.row[c + 1]) + kCell / 2;
            const int y1 = y + kCell / 2;
            const int xm = x + kCell;
            g.fillRect(xm, y1 < y2 ? y1 : y2, kGap, (y1 < y2 ? y2 - y1 : y1 - y2) + 1,
                       A(P::magenta));
        }
    }
}

void drawLever(lgfx::LGFX_Sprite& g, const core::VideoGame& game, uint32_t now) {
    float pull = 0.0f;
    if (game.phase == core::Phase::Spinning) {
        const uint32_t dt = now - game.phaseT0;
        pull = dt < 120 ? 1.0f : (dt < 600 ? 1.0f - (dt - 120) / 480.0f : 0.0f);
    }
    const int top = kLeverTop + static_cast<int>(pull * kLeverTravel);
    g.fillRect(kLeverCx - 2, top + 10, 4, kLeverBaseY - top - 10, P::steel500);
    g.fillRect(kLeverCx - 1, top + 10, 1, kLeverBaseY - top - 10, P::steel300);
    if (g_demo) drawIconGray(g, ICON_BALL, kLeverCx - 6, top);
    else drawIcon(g, ICON_BALL, kLeverCx - 6, top);
    g.fillRect(kLeverCx - 9, kLeverBaseY, 18, 3, P::ink600);
    g.fillRect(kLeverCx - 11, kLeverBaseY + 3, 20, 8, P::steel500);
    drawFrame(g, kLeverCx - 11, kLeverBaseY + 3, 20, 8, P::ink900, 1);
}

void drawTraces(lgfx::LGFX_Sprite& g) {
    static const int16_t kSeg[][4] = {
        {4, 24, 2, 40}, {4, 62, 8, 2}, {10, 64, 2, 34}, {4, 100, 2, 16},
        {230, 22, 2, 30}, {220, 20, 12, 2}, {230, 112, 2, 14},
    };
    for (auto& s : kSeg) g.fillRect(s[0], s[1], s[2], s[3], P::ink700);
    static const int16_t kVia[][2] = {{3, 61}, {9, 97}, {229, 51}, {229, 111}};
    for (auto& v : kVia) g.fillRect(v[0], v[1], 4, 4, P::ink600);
}

void drawMessage(lgfx::LGFX_Sprite& g, const core::VideoGame& game, uint32_t now) {
    const int ty = kMsgY;
    if (g_demo) {
        drawText(g, "DEMO", kScreenW / 2, ty, kGrayLight, 2, Align::Center);
        return;
    }
    switch (game.phase) {
        case core::Phase::Idle:
            drawText(g, "SHAKE TO SPIN", kScreenW / 2, ty, P::cyan, 2, Align::Center);
            break;
        case core::Phase::Spinning:
            drawText(g, "SPINNING", kScreenW / 2, ty, P::magenta, 2, Align::Center);
            break;
        case core::Phase::Celebrate: {
            if (game.tier == core::Tier::Jackpot && blink(now, 300)) {
                drawText(g, "JACKPOT", kScreenW / 2, ty, P::green, 2, Align::Center);
            } else if (game.tier != core::Tier::Jackpot) {
                const int wl = textWidth("WIN ", 2);
                const int wn = numberWidth(static_cast<int32_t>(game.payout), 2);
                const int x0 = (kScreenW - wl - wn) / 2;
                drawText(g, "WIN", x0, ty, P::yellow, 2);
                drawNumber(g, static_cast<int32_t>(game.payout), x0 + wl, ty, P::white, 2);
            }
            break;
        }
        case core::Phase::Bailout:
            drawText(g, "THE HOUSE REFILLS +500", kScreenW / 2, ty + 3, P::green, 1,
                     Align::Center);
            break;
    }
}

}  // namespace

void drawVideoScreen(lgfx::LGFX_Sprite& g, const core::VideoGame& game,
                     uint32_t now, bool classic) {
    g_demo = game.attract;
    g.fillScreen(P::ink900);
    drawTraces(g);
    drawOverlayHud(g, game);

    const uint8_t hot = core::highlightedLine(game, now);
    drawChevrons(g, game, now, hot);

    const core::ReelSet& rs = *game.reels;
    for (uint8_t c = 0; c < core::kVideoReels; ++c) {
        // Fond de colonne : juste assez pour détacher la grille du vide.
        g.fillRect(cellX(c), kGridY, kCell, kGridH, P::ink800);

        const float p = core::videoReelPos(game, c, now);
        const float speed = p - core::videoReelPos(
            game, c, now > core::kFrameMs ? now - core::kFrameMs : 0);

        g.setClipRect(cellX(c), kGridY, kCell, kGridH);
        if (speed > kBlurThreshold) {
            drawBlurredColumn(g, rs, c, p, classic, g_demo);
        } else {
            const float base = std::floor(p);
            const int shift = static_cast<int>((p - base) * kCell);
            for (int k = -1; k <= kRows; ++k) {
                const uint8_t sym = core::symbolAt(rs, c, static_cast<int32_t>(base) + k);
                const int y = cellY(0) + k * kCell - shift;
                if (g_demo) drawSymbolGray(g, sym, cellX(c), y, kScale, classic);
                else drawSymbol(g, sym, cellX(c), y, kScale, 0, classic);
            }
        }
        g.clearClipRect();
    }

    drawWinPath(g, game, hot, now);
    drawLever(g, game, now);
    drawMessage(g, game, now);
}

}  // namespace ui
