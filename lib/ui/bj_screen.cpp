#include "bj_screen.h"

#include "layout.h"
#include "painter.h"
#include "palette.h"

namespace ui {

using namespace bjlayout;
using layout::kScreenW;
using layout::kScreenH;
namespace P = pal;

namespace {

bool blink(uint32_t now, uint32_t periodMs) {
    return ((now / (periodMs / 2)) & 1u) != 0;
}

const char* rankLabel(uint8_t rank) {
    switch (rank) {
        case 1: return "A";
        case 10: return "10";
        case 11: return "J";
        case 12: return "Q";
        case 13: return "K";
        default: return nullptr;  // chiffre : dessiné en nombre
    }
}

// Enseigne rouge ou noire — ici « noire » veut dire acier clair : du vrai
// noir sur fond sombre ne se verrait pas.
uint16_t suitColor(uint8_t suit) {
    return (suit == SUIT_HEART || suit == SUIT_DIAMOND) ? P::red : P::ink900;
}

}  // namespace

void drawCard(lgfx::LGFX_Sprite& g, core::Card c, int x, int y, bool faceDown) {
    if (faceDown) {
        // Dos de carte : motif de circuit, cohérent avec le reste du casino.
        g.fillRect(x, y, kCardW, kCardH, P::violetDk);
        drawFrame(g, x, y, kCardW, kCardH, P::violet, 1);
        for (int i = 4; i < kCardH - 4; i += 5) {
            g.fillRect(x + 4, y + i, kCardW - 8, 2, P::violet);
        }
        g.fillRect(x + kCardW / 2 - 2, y + 4, 4, kCardH - 8, P::violetDk);
        return;
    }

    g.fillRect(x, y, kCardW, kCardH, P::steel300);
    drawFrame(g, x, y, kCardW, kCardH, P::white, 1);
    const uint16_t col = suitColor(c.suit);

    const char* lbl = rankLabel(c.rank);
    if (lbl) drawText(g, lbl, x + 3, y + 3, col, 2);
    else drawNumber(g, c.rank, x + 3, y + 3, col, 2);

    // L'enseigne occupe le bas de la carte : le rang se lit en haut, comme
    // sur une vraie carte tenue en éventail.
    blitSuit(g, c.suit, x + kCardW - 11, y + kCardH - 11);
}

namespace {

void drawHand(lgfx::LGFX_Sprite& g, const core::Hand& h, uint8_t visible, int y,
              bool hideSecond) {
    const int step = visible > 4 ? kCardStepTight : kCardStep;
    const int w = visible > 0 ? (visible - 1) * step + kCardW : 0;
    const int x0 = (kScreenW - 40 - w) / 2 + 10;
    for (uint8_t i = 0; i < visible && i < h.n; ++i) {
        drawCard(g, h.c[i], x0 + i * step, y, hideSecond && i == 1);
    }
}

void drawTotal(lgfx::LGFX_Sprite& g, const core::Hand& h, uint8_t visible, int y,
               bool hidden, uint16_t color) {
    if (hidden || visible == 0) {
        drawText(g, "?", kScreenW - 20, y + kCardH / 2 - 7, P::steel500, 2,
                 Align::Center);
        return;
    }
    core::Hand shown;
    core::handClear(shown);
    for (uint8_t i = 0; i < visible && i < h.n; ++i) core::handAdd(shown, h.c[i]);
    const core::HandValue v = core::handValue(shown);
    drawNumber(g, v.total, kScreenW - 20, y + kCardH / 2 - 7,
               v.total > 21 ? P::red : color, 2, Align::Center);
    if (v.soft) drawText(g, "S", kScreenW - 20, y + kCardH / 2 + 5, P::cyan, 1,
                         Align::Center);
}

const char* outcomeText(core::BjOutcome o) {
    switch (o) {
        case core::BjOutcome::PlayerBlackjack: return "BLACKJACK!";
        case core::BjOutcome::PlayerWin: return "YOU WIN";
        case core::BjOutcome::DealerBust: return "DEALER BUST";
        case core::BjOutcome::PlayerBust: return "BUST";
        case core::BjOutcome::DealerWin: return "DEALER WINS";
        case core::BjOutcome::Push: return "PUSH";
        default: return "";
    }
}

uint16_t outcomeColor(core::BjOutcome o) {
    switch (o) {
        case core::BjOutcome::PlayerBlackjack: return P::green;
        case core::BjOutcome::PlayerWin:
        case core::BjOutcome::DealerBust: return P::yellow;
        case core::BjOutcome::Push: return P::cyan;
        default: return P::red;
    }
}

void drawActions(lgfx::LGFX_Sprite& g, const core::BjSession& s, uint32_t now) {
    static const char* kNames[core::kBjChoices] = {"HIT", "STAND", "DOUBLE"};
    const bool canDouble = core::bjCanDouble(s.bj, s.econ);

    // Conseil de stratégie de base, si le réglage est actif.
    core::BjAction hint = core::BjAction::Stand;
    if (s.hintsOn && s.bj.player.n >= 2) {
        hint = core::bjBasicStrategy(s.bj.player, s.bj.dealer.c[0], canDouble);
    }

    int x = 8;
    for (uint8_t i = 0; i < core::kBjChoices; ++i) {
        const bool avail = (i != 2) || canDouble;
        const bool sel = static_cast<uint8_t>(s.choice) == i;
        const int w = textWidth(kNames[i], 2) + 10;
        if (sel) {
            g.fillRect(x, kActionsY, w, 18, P::ink700);
            drawFrame(g, x, kActionsY, w, 18, P::cyan, 1);
        }
        drawText(g, kNames[i], x + 5, kActionsY + 4,
                 avail ? (sel ? P::white : P::steel300) : P::ink600, 2);
        // Le conseil se signale par un point, pas par un texte : il informe
        // sans dicter.
        if (s.hintsOn && avail && static_cast<uint8_t>(hint) == i) {
            g.fillRect(x + w - 5, kActionsY + 2, 3, 3, P::green);
        }
        x += w + 5;
    }
}

}  // namespace

void drawBjScreen(lgfx::LGFX_Sprite& g, const core::BjSession& s, uint32_t now) {
    g.fillScreen(P::ink900);

    // HUD en surimpression, comme partout ailleurs maintenant.
    drawIcon(g, ICON_COIN, 3, 3);
    drawNumber(g, s.econ.credits, 19, 3, P::yellow, 2);
    const int bw = numberWidth(s.bj.stake ? s.bj.stake : core::bet(s.econ), 2);
    drawNumber(g, s.bj.stake ? s.bj.stake : core::bet(s.econ), kScreenW - 3, 3,
               s.bj.doubled ? P::magenta : P::cyan, 2, Align::Right);
    drawText(g, "BET", kScreenW - 9 - bw, 3, P::steel300, 2, Align::Right);

    const bool hidden = core::bjHoleHidden(s);
    drawHand(g, s.bj.dealer, core::bjVisibleDealer(s), kDealerY, hidden);
    drawTotal(g, s.bj.dealer, core::bjVisibleDealer(s), kDealerY, hidden, P::steel300);
    drawHand(g, s.bj.player, core::bjVisiblePlayer(s), kPlayerY, false);
    drawTotal(g, s.bj.player, core::bjVisiblePlayer(s), kPlayerY, false, P::white);

    if (s.revealed < 4) {
        drawText(g, "DEALING", kScreenW / 2, kActionsY + 4, P::steel300, 2,
                 Align::Center);
    } else if (s.bj.phase == core::BjPhase::PlayerTurn) {
        drawActions(g, s, now);
    } else if (s.bj.phase == core::BjPhase::DealerTurn) {
        drawText(g, "DEALER PLAYS", kScreenW / 2, kActionsY + 4, P::cyan, 2,
                 Align::Center);
    } else if (s.bj.phase == core::BjPhase::Settle) {
        const char* t = outcomeText(s.bj.outcome);
        const uint16_t c = outcomeColor(s.bj.outcome);
        const bool big = s.bj.outcome == core::BjOutcome::PlayerBlackjack;
        if (!big || blink(now, 320)) {
            drawText(g, t, kScreenW / 2, kActionsY + 4, c, 2, Align::Center);
        }
        if (s.bj.payout > 0) {
            drawText(g, "+", 8, kActionsY + 4, P::yellow, 2);
            drawNumber(g, static_cast<int32_t>(s.bj.payout), 20, kActionsY + 4,
                       P::yellow, 2);
        }
    } else {
        drawText(g, "PRESS SPACE TO DEAL", kScreenW / 2, kActionsY + 4, P::cyan, 2,
                 Align::Center);
    }
}

}  // namespace ui
