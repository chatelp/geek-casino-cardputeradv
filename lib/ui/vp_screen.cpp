#include "vp_screen.h"

#include "bj_screen.h"  // drawCard, partagé — un seul rendu de carte
#include "layout.h"
#include "painter.h"
#include "palette.h"

namespace ui {

using namespace vplayout;
using bjlayout::kCardH;
using bjlayout::kCardW;
using layout::kScreenH;
using layout::kScreenW;
namespace P = pal;

namespace {

// Mode démo : tout passe en gris clair, comme aux machines à sous. Le
// monochrome EST le message — inutile d'un bandeau clignotant.
bool g_demo = false;
uint16_t A(uint16_t accent) { return g_demo ? kGrayLight : accent; }
uint16_t D(uint16_t dim) { return g_demo ? kGrayMid : dim; }

bool blink(uint32_t now, uint32_t periodMs) {
    return ((now / (periodMs / 2)) & 1u) != 0;
}

// Même carte-circuit que le blackjack : le décor porte le geek, les
// figures restent lisibles (règle de projet).
void drawTable(lgfx::LGFX_Sprite& g) {
    constexpr int kTx = 4, kTy = 20, kTw = kScreenW - 8, kTh = 62;
    g.fillRect(kTx, kTy, kTw, kTh, P::pcb);
    drawFrame(g, kTx, kTy, kTw, kTh, P::pcbEdge, 1);

    for (int y = kTy + 6; y < kTy + kTh - 4; y += 13) {
        const bool odd = ((y - kTy) / 13) & 1;
        for (int x = kTx + (odd ? 12 : 6); x < kTx + kTw - 4; x += 13) {
            drawVia(g, x, y, P::pcbLine, P::pcb);
        }
    }
    // Un bus court sous chaque emplacement de carte : la rangée de cinq
    // se lit comme une barrette de composants.
    for (int i = 0; i < core::kPokerHandSize; ++i) {
        const int x = cardX(i);
        g.fillRect(x + 4, kTy + kTh - 6, kCardW - 8, 1, P::pcbEdge);
        g.fillRect(x + kCardW / 2 - 1, kTy + kTh - 9, 2, 3, P::tan);
    }
    trace45(g, 8, kTy + 8, 26, kTy + 26, P::pcbEdge, 1);
    trace45(g, kScreenW - 9, kTy + 8, kScreenW - 27, kTy + 26, P::pcbEdge, 1);
}

void drawHud(lgfx::LGFX_Sprite& g, const core::VpSession& s) {
    drawIcon(g, ICON_COIN, 3, 3);
    drawNumber(g, s.econ.credits, 19, 3, A(P::yellow), 2);

    const bool between = s.phase != core::VpPhase::Holding;
    const int32_t shown = between ? core::bet(s.econ) : s.stake;
    const int bw = numberWidth(shown, 2);
    // Mise maximale en magenta : c'est elle qui fait passer la royale de
    // 250 à 800, l'information doit sauter aux yeux.
    const bool maxi = between ? core::vpIsMaxBet(s.econ) : s.maxBet;
    drawNumber(g, shown, kScreenW - 8, 3, A(maxi ? P::magenta : P::cyan), 2,
               Align::Right);
    drawText(g, "BET", kScreenW - 14 - bw, 3, P::steel300, 2, Align::Right);
    drawBetArrows(g, kScreenW - 14 - bw - 10, kScreenW - 6, 4, P::steel500,
                  between && s.econ.betIndex > 0,
                  between && s.econ.betIndex + 1 < core::kBetSteps);
}

void drawCards(lgfx::LGFX_Sprite& g, const core::VpSession& s, uint32_t now) {
    const uint8_t visible = core::vpVisible(s);
    for (int i = 0; i < core::kPokerHandSize; ++i) {
        const int x = cardX(i);
        if (i >= visible) {
            // Emplacement vide : empreinte sérigraphiée, comme au blackjack.
            drawFrame(g, x, kCardsY, kCardW, kCardH, P::pcbEdge, 1);
            continue;
        }
        drawCard(g, s.hand.c[i], x, kCardsY, false, g_demo);

        if (s.held[i]) {
            // « HELD » sous la carte, plus un cadre : le joueur doit voir
            // d'un coup d'œil ce qu'il garde, sans compter.
            drawFrame(g, x - 2, kCardsY - 2, kCardW + 4, kCardH + 4, A(P::yellow), 2);
            const int w = textWidth("HELD", 1) + 6;
            g.fillRect(x + (kCardW - w) / 2, kHeldY, w, 9, A(P::yellow));
            drawText(g, "HELD", x + kCardW / 2, kHeldY + 1, P::ink900, 1,
                     Align::Center);
        }
        if (s.phase == core::VpPhase::Holding && s.cursor == i &&
            visible >= core::kPokerHandSize && blink(now, 500)) {
            drawFrame(g, x - 4, kCardsY - 4, kCardW + 8, kCardH + 8, P::cyan, 1);
        }
    }
}

void drawActions(lgfx::LGFX_Sprite& g, const core::VpSession& s, uint32_t now) {
    if (s.phase != core::VpPhase::Holding) return;
    if (core::vpVisible(s) < core::kPokerHandSize) {
        drawText(g, "DEALING", kScreenW / 2, kActionY + 3, P::steel300, 2,
                 Align::Center);
        return;
    }
    const bool onDraw = s.cursor == core::kVpDrawSlot;
    const int w = textWidth("DRAW", 2) + 14;
    const int x = (kScreenW - w) / 2;
    g.fillRect(x, kActionY, w, 18, onDraw ? P::cyan : P::ink700);
    drawFrame(g, x, kActionY, w, 18, onDraw ? P::white : P::steel500, 1);
    drawText(g, "DRAW", kScreenW / 2, kActionY + 4,
             onDraw ? P::ink900 : P::steel300, 2, Align::Center);
    if (onDraw && blink(now, 500)) {
        drawFrame(g, x - 3, kActionY - 3, w + 6, 24, P::cyan, 1);
    }
    // Pas d'invite en démo : « DEMO » occupe déjà cette ligne.
    if (!g_demo) {
        drawText(g, "</> PICK   ENTER HOLD/DRAW", kScreenW / 2, kMsgY + 6,
                 P::steel500, 1, Align::Center);
    }
}

void drawResult(lgfx::LGFX_Sprite& g, const core::VpSession& s, uint32_t now) {
    if (g_demo) {
        drawText(g, "DEMO", kScreenW / 2, kMsgY + 4, kGrayLight, 2, Align::Center);
        return;
    }
    if (s.phase == core::VpPhase::Idle) {
        drawText(g, "PRESS SPACE TO DEAL", kScreenW / 2, kMsgY + 4, P::cyan, 2,
                 Align::Center);
        return;
    }
    if (s.phase != core::VpPhase::Result) return;

    if (s.result == core::PokerRank::None) {
        drawText(g, "NO WIN", kScreenW / 2, kMsgY + 4, P::steel500, 2,
                 Align::Center);
        drawText(g, "SPACE FOR A NEW HAND", kScreenW / 2, kMsgY + 20,
                 P::steel500, 1, Align::Center);
        return;
    }
    const bool huge = s.result >= core::PokerRank::StraightFlush;
    const uint16_t col = huge ? P::green
                        : (s.result >= core::PokerRank::FullHouse ? P::yellow
                                                                  : P::cyan);
    if (!huge || blink(now, 320)) {
        drawText(g, core::pokerRankName(s.result), kScreenW / 2, kMsgY + 2, col,
                 1, Align::Center);
    }
    const int wl = textWidth("WIN ", 2);
    const int wn = numberWidth(static_cast<int32_t>(s.payout), 2);
    const int x0 = (kScreenW - wl - wn) / 2;
    drawText(g, "WIN", x0, kMsgY + 12, P::yellow, 2);
    drawNumber(g, static_cast<int32_t>(s.payout), x0 + wl, kMsgY + 12, P::white, 2);
}

}  // namespace

void drawVpScreen(lgfx::LGFX_Sprite& g, const core::VpSession& s, uint32_t now) {
    g_demo = s.attract;
    g.fillScreen(P::ink900);
    drawTable(g);
    drawHud(g, s);
    drawCards(g, s, now);
    drawActions(g, s, now);
    drawResult(g, s, now);
}

}  // namespace ui
