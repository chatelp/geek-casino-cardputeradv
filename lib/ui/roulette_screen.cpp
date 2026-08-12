#include "roulette_screen.h"

#include <cmath>

#include "layout.h"
#include "painter.h"
#include "palette.h"

namespace ui {

using namespace rlayout;
using layout::kScreenH;
using layout::kScreenW;
namespace P = pal;

namespace {

bool blink(uint32_t now, uint32_t periodMs) {
    return ((now / (periodMs / 2)) & 1u) != 0;
}

uint16_t pocketColour(uint8_t n) {
    if (n == 0) return P::greenDk;
    return core::isRed(n) ? P::magentaDk : P::ink700;
}

// La roue, en bande. Le geek passe par le décor : la bande est montée sur
// un encodeur rotatif — crans entre les cases, axe et corps sous la
// bande. Les numéros restent des numéros (règle de projet).
void drawWheel(lgfx::LGFX_Sprite& g, const core::RouletteSession& s, uint32_t now) {
    const float p = core::rltWheelPos(s, now);
    const float base = std::floor(p);
    const int shift = static_cast<int>((p - base) * kCellW);

    g.setClipRect(kStripX, kStripY, kVisible * kCellW, kStripH);
    for (int k = -1; k <= kVisible; ++k) {
        // La case centrale est celle sous le repère : décalage de 2.
        const int32_t idx = static_cast<int32_t>(base) + k - 2;
        const int32_t wrapped = ((idx % core::kPockets) + core::kPockets) % core::kPockets;
        const uint8_t n = core::pocketAt(static_cast<uint8_t>(wrapped));
        const int x = cellLeft(k) - shift;

        g.fillRect(x + 1, kStripY + 2, kCellW - 2, kStripH - 4, pocketColour(n));
        drawFrame(g, x + 1, kStripY + 2, kCellW - 2, kStripH - 4, P::ink900, 1);
        // La case sous le repère se détache : sans ça, l'œil doit suivre
        // les chevrons pour savoir laquelle compte.
        if (k == 2) {
            drawFrame(g, x + 1, kStripY + 2, kCellW - 2, kStripH - 4, P::white, 1);
        }
        drawNumber(g, n, x + kCellW / 2, kStripY + kStripH / 2 - 7, P::white, 2,
                   Align::Center);
        // Cran d'encodeur entre deux cases.
        g.fillRect(x, kStripY, 1, kStripH, P::steel500);
    }
    g.clearClipRect();

    drawFrame(g, kStripX, kStripY, kVisible * kCellW, kStripH, P::steel500, 1);

    // Repère de lecture : deux chevrons pointant la case centrale.
    const int cx = kScreenW / 2;
    const uint16_t mark = s.phase == core::RltPhase::Spinning ? P::yellow : P::cyan;
    for (int k = 0; k < 5; ++k) {
        g.fillRect(cx - 4 + k, kStripY - 6 + k, 9 - 2 * k, 1, mark);
        g.fillRect(cx - 4 + k, kStripY + kStripH + 5 - k, 9 - 2 * k, 1, mark);
    }
}

// Corps de l'encodeur sous la bande : axe, méplat, et deux pattes.
void drawEncoder(lgfx::LGFX_Sprite& g) {
    const int cx = kScreenW / 2;
    const int y = kStripY + kStripH + 8;
    g.fillRect(cx - 18, y, 36, 4, P::steel500);
    g.fillRect(cx - 22, y + 4, 44, 2, P::ink600);
    for (int i = 0; i < 4; ++i) {
        g.fillRect(cx - 15 + i * 10, y - 3, 2, 3, P::tan);
    }
}

void drawHud(lgfx::LGFX_Sprite& g, const core::RouletteSession& s) {
    drawIcon(g, ICON_COIN, 3, 3);
    drawNumber(g, s.econ.credits, 19, 3, P::yellow, 2);
    const bool idle = s.phase != core::RltPhase::Spinning;
    const int32_t shown = idle ? core::bet(s.econ) : s.stake;
    const int bw = numberWidth(shown, 2);
    drawNumber(g, shown, kScreenW - 8, 3, P::cyan, 2, Align::Right);
    drawText(g, "BET", kScreenW - 14 - bw, 3, P::steel300, 2, Align::Right);
    drawBetArrows(g, kScreenW - 14 - bw - 10, kScreenW - 6, 4, P::steel500,
                  idle && s.econ.betIndex > 0,
                  idle && s.econ.betIndex + 1 < core::kBetSteps);
}

// Le pari choisi, avec son gain. Un seul pari à la fois : placer des
// jetons sur un tapis demanderait un curseur en deux dimensions, que ce
// clavier ne rend pas agréable.
void drawBetPanel(lgfx::LGFX_Sprite& g, const core::RouletteSession& s) {
    const bool straight = s.kind == core::BetKind::Straight;
    const uint16_t col = s.kind == core::BetKind::Red ? P::magenta
                       : (s.kind == core::BetKind::Black ? P::steel300 : P::cyan);

    g.fillRect(6, kBetY, kScreenW - 12, kBetH, P::ink800);
    drawFrame(g, 6, kBetY, kScreenW - 12, kBetH, P::ink600, 1);
    drawText(g, "BET ON", 12, kBetY + 3, P::steel500, 1);
    drawText(g, "PAYS", kScreenW - 12, kBetY + 3, P::steel500, 1, Align::Right);
    drawNumber(g, core::roulettePayout(s.kind), kScreenW - 12, kBetY + 12,
               P::yellow, 2, Align::Right);

    if (straight) {
        drawNumber(g, s.straight, 12, kBetY + 12, P::white, 2);
        // Pastille de couleur du numéro : rouge, noir, ou vert pour le zéro.
        const int nx = 12 + numberWidth(s.straight, 2) + 6;
        g.fillRect(nx, kBetY + 12, 12, 12, pocketColour(s.straight));
        // Chevrons DESSINÉS : la fonte n'a pas de « ^ », l'écrire donnait
        // un blanc et le repère disparaissait.
        drawChevronV(g, nx + 20, kBetY + 13, false, P::cyan);
        drawChevronV(g, nx + 20, kBetY + 18, true, P::cyan);
        drawText(g, "PICK", nx + 28, kBetY + 15, P::steel500, 1);
    } else {
        drawText(g, core::betName(s.kind), 12, kBetY + 12, col, 2);
    }
}

void drawMessage(lgfx::LGFX_Sprite& g, const core::RouletteSession& s, uint32_t now) {
    switch (s.phase) {
        case core::RltPhase::Idle:
            drawText(g, "</> BET   SPACE TO SPIN", kScreenW / 2, kMsgY + 2,
                     P::cyan, 1, Align::Center);
            break;
        case core::RltPhase::Spinning:
            drawText(g, "NO MORE BETS", kScreenW / 2, kMsgY, P::magenta, 2,
                     Align::Center);
            break;
        case core::RltPhase::Result: {
            if (s.won) {
                const int wl = textWidth("WIN ", 2);
                const int wn = numberWidth(static_cast<int32_t>(s.payout), 2);
                const int x0 = (kScreenW - wl - wn) / 2;
                const bool big = s.kind == core::BetKind::Straight;
                if (!big || blink(now, 300)) {
                    drawText(g, "WIN", x0, kMsgY, P::yellow, 2);
                    drawNumber(g, static_cast<int32_t>(s.payout), x0 + wl, kMsgY,
                               P::white, 2);
                }
            } else if (s.winNumber == 0) {
                // Le zéro mérite d'être nommé : c'est là que la maison gagne.
                drawText(g, "ZERO - HOUSE TAKES ALL", kScreenW / 2, kMsgY + 2,
                         P::greenDk, 1, Align::Center);
            } else {
                drawText(g, "NO WIN", kScreenW / 2, kMsgY, P::steel500, 2,
                         Align::Center);
            }
            break;
        }
    }
}

}  // namespace

void drawRouletteScreen(lgfx::LGFX_Sprite& g, const core::RouletteSession& s,
                        uint32_t now) {
    g.fillScreen(P::ink900);
    drawHud(g, s);
    drawWheel(g, s, now);
    drawEncoder(g);
    drawBetPanel(g, s);
    drawMessage(g, s, now);
}

}  // namespace ui
