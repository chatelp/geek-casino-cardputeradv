#include "bj_screen.h"

#include "celebration.h"
#include "layout.h"
#include "painter.h"
#include "palette.h"

namespace ui {

using namespace bjlayout;
using layout::kScreenW;
using layout::kScreenH;
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

void drawCard(lgfx::LGFX_Sprite& g, core::Card c, int x, int y, bool faceDown,
              bool demo) {
    if (faceDown) {
        // Le dos est ce qu'on voit le plus souvent : c'est lui qui porte
        // l'identité. Circuit à vias, invader en médaillon — dessiné dans
        // le design system, pas bricolé ici.
        if (demo) drawCardBackGray(g, x, y, 2);
        else drawCardBack(g, x, y, 2);
        return;
    }

    g.fillRect(x, y, kCardW, kCardH, demo ? kGrayMid : P::steel300);
    drawFrame(g, x, y, kCardW, kCardH, demo ? kGrayLight : P::white, 1);
    const uint16_t col = demo ? P::ink900 : suitColor(c.suit);

    const char* lbl = rankLabel(c.rank);
    if (lbl) drawText(g, lbl, x + 3, y + 3, col, 2);
    else drawNumber(g, c.rank, x + 3, y + 3, col, 2);

    // L'enseigne occupe le bas de la carte : le rang se lit en haut, comme
    // sur une vraie carte tenue en éventail.
    blitSuit(g, c.suit, x + kCardW - 11, y + kCardH - 11, 1,
             demo ? P::ink900 : 0);
}

namespace {

void drawHand(lgfx::LGFX_Sprite& g, const core::Hand& h, uint8_t visible, int y,
              bool hideSecond) {
    const int step = visible > 4 ? kCardStepTight : kCardStep;
    const int w = visible > 0 ? (visible - 1) * step + kCardW : 0;
    const int x0 = (kScreenW - 40 - w) / 2 + 10;
    for (uint8_t i = 0; i < visible && i < h.n; ++i) {
        drawCard(g, h.c[i], x0 + i * step, y, hideSecond && i == 1, g_demo);
    }
}

void drawTotal(lgfx::LGFX_Sprite& g, const core::Hand& h, uint8_t visible, int y,
               bool hidden, uint16_t color) {
    if (visible == 0) return;  // table vide : rien à totaliser
    if (hidden) {
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

namespace {

// La table de blackjack EST une carte électronique : le tapis vert du
// casino et le vernis épargne d'un PCB sont exactement le même vert.
// C'est ce jeu de mots qui apporte le geek sans toucher aux figures.
void drawTable(lgfx::LGFX_Sprite& g) {
    constexpr int kTx = 4, kTy = 18, kTw = kScreenW - 8, kTh = 88;

    // Vernis épargne, puis bord de carte plus clair : une carte a une
    // tranche visible, pas un simple trait.
    g.fillRect(kTx, kTy, kTw, kTh, P::pcb);
    drawFrame(g, kTx, kTy, kTw, kTh, P::pcbEdge, 1);
    drawFrame(g, kTx + 1, kTy + 1, kTw - 2, kTh - 2, P::pcbLine, 1);

    // Maillage de vias du plan de masse : une grille de perçages cousant
    // le cuivre. C'est ce motif-là qu'on reconnaît sur une vraie carte,
    // bien plus qu'un aplat.
    for (int y = kTy + 8; y < kTy + kTh - 6; y += 13) {
        const bool odd = ((y - kTy) / 13) & 1;
        for (int x = kTx + (odd ? 13 : 7); x < kTx + kTw - 6; x += 13) {
            // Zone d'exclusion sous la sérigraphie : un fondeur ne place
            // pas de via sous un marquage, et le texte y gagne en lisibilité.
            if (x < 74 && y > 20 && y < 100) continue;
            drawVia(g, x, y, P::pcbLine, P::pcb);
        }
    }

    // Trous de fixation aux angles.
    const int hx[2] = {kTx + 3, kTx + kTw - 9};
    const int hy[2] = {kTy + 3, kTy + kTh - 9};
    for (int a = 0; a < 2; ++a) {
        for (int b = 0; b < 2; ++b) {
            g.fillRect(hx[a], hy[b], 6, 6, P::steel500);
            g.fillRect(hx[a] + 2, hy[b] + 2, 2, 2, P::ink900);
        }
    }

    // Pistes routées, toutes cassées à 45°. Elles relient des pastilles
    // dorées — la finition ENIG d'une carte réelle.
    const int mid = kPlayerY - 6;
    struct Route { int16_t x0, y0, x1, y1; };
    // Routage tenu HORS de la zone de sérigraphie (x < 76, y 22..86) :
    // une piste qui traverse un marquage le rend illisible, sur écran
    // comme sur cuivre.
    static const Route kRoutes[] = {
        {226, 34, 196, 64}, {226, 52, 202, 76}, {212, 92, 190, 100},
        {150, 100, 182, 100}, {92, 100, 120, 100}, {14, 94, 44, 100},
    };
    for (const auto& r : kRoutes) {
        trace45(g, r.x0, r.y0, r.x1, r.y1, P::pcbEdge, 1);
        g.fillRect(r.x0 - 1, r.y0 - 1, 3, 3, P::tan);   // pastille dorée
        g.fillRect(r.x1 - 1, r.y1 - 1, 3, 3, P::tan);
    }

    // Bus horizontal séparant croupier et joueur — l'arc de la table
    // réelle, tracé comme une piste large et ponctué de vias.
    g.fillRect(kTx + 12, mid, kTw - 24, 2, P::pcbEdge);
    for (int x = kTx + 22; x < kTx + kTw - 20; x += 30) {
        drawVia(g, x, mid - 1, P::tan, P::pcb);
    }

    // Empreintes de composants là où les cartes se posent : une table de
    // casino a ses emplacements sérigraphiés, un circuit aussi.
    for (int row = 0; row < 2; ++row) {
        const int y = row == 0 ? kDealerY : kPlayerY;
        for (int i = 0; i < 2; ++i) {
            const int x = (kScreenW - 40 - (kCardStep + kCardW)) / 2 + 10 + i * kCardStep;
            drawFrame(g, x - 2, y - 2, kCardW + 4, kCardH + 4, P::pcbEdge, 1);
            g.fillRect(x - 2, y - 2, 3, 1, P::tan);   // repère broche 1
            g.fillRect(x - 2, y - 2, 1, 3, P::tan);
            // Pastilles de brasage de part et d'autre de l'empreinte.
            for (int k = 0; k < 3; ++k) {
                g.fillRect(x - 5, y + 6 + k * 12, 3, 5, P::tan);
                g.fillRect(x + kCardW + 2, y + 6 + k * 12, 3, 5, P::tan);
            }
        }
    }
}

// Sérigraphie : le texte réglementaire d'une table de casino, rendu comme
// un marquage de circuit. Échelle 1 assumée — une sérigraphie est petite
// par nature, et c'est du décor, pas une information dont le jeu dépend.
void drawSilkscreen(lgfx::LGFX_Sprite& g) {
    // Bloc en marge gauche : c'est la seule bande libre, et c'est
    // exactement là qu'un vrai circuit porte son marquage. Les cartes
    // occupent le centre à partir de x=80.
    static const char* kMarks[] = {
        "DEALER", "STANDS ON", "ALL 17", "", "BJ PAYS", "3:2",
    };
    constexpr int kMarkCount = sizeof(kMarks) / sizeof(kMarks[0]);
    for (int i = 0; i < kMarkCount; ++i) {
        if (kMarks[i][0] == '\0') continue;
        drawText(g, kMarks[i], 9, 24 + i * 10,
                 i < 3 ? P::pcbEdge : P::pcbLine, 1);
    }
    drawText(g, "REV 1.0", 9, 84, P::pcbLine, 1);
}

}  // namespace

void drawBjScreen(lgfx::LGFX_Sprite& g, const core::BjSession& s, uint32_t now) {
    g_demo = s.attract;
    g.fillScreen(P::ink900);
    drawTable(g);
    drawSilkscreen(g);

    // HUD en surimpression, comme partout ailleurs maintenant.
    drawIcon(g, ICON_COIN, 3, 3);
    drawNumber(g, s.econ.credits, 19, 3, A(P::yellow), 2);
    // Entre deux mains on montre la mise réglable ; pendant la main, la
    // mise engagée (doublée le cas échéant, en magenta).
    const bool between = s.bj.phase != core::BjPhase::PlayerTurn &&
                         s.bj.phase != core::BjPhase::DealerTurn;
    const int32_t shown = between ? core::bet(s.econ) : s.bj.stake;
    const int bw = numberWidth(shown, 2);
    drawNumber(g, shown, kScreenW - 8, 3,
               A(s.bj.doubled && !between ? P::magenta : P::cyan), 2, Align::Right);
    drawText(g, "BET", kScreenW - 14 - bw, 3, P::steel300, 2, Align::Right);
    drawBetArrows(g, kScreenW - 14 - bw - 10, kScreenW - 6, 4, P::steel500,
                  between && s.econ.betIndex > 0,
                  between && s.econ.betIndex + 1 < core::kBetSteps);

    const bool hidden = core::bjHoleHidden(s);
    drawHand(g, s.bj.dealer, core::bjVisibleDealer(s), kDealerY, hidden);
    drawTotal(g, s.bj.dealer, core::bjVisibleDealer(s), kDealerY, hidden, P::steel300);
    drawHand(g, s.bj.player, core::bjVisiblePlayer(s), kPlayerY, false);
    drawTotal(g, s.bj.player, core::bjVisiblePlayer(s), kPlayerY, false, P::white);

    if (s.bj.phase != core::BjPhase::Idle && s.revealed < 4) {
        drawText(g, "DEALING", kScreenW / 2, kActionsY + 4, P::steel300, 2,
                 Align::Center);
    } else if (s.bj.phase == core::BjPhase::PlayerTurn && s.revealed >= 4) {
        drawActions(g, s, now);
    } else if (s.bj.phase == core::BjPhase::DealerTurn) {
        drawText(g, "DEALER PLAYS", kScreenW / 2, kActionsY + 4, P::cyan, 2,
                 Align::Center);
    } else if (s.bj.phase == core::BjPhase::Settle) {
        // Les mains perdantes gardent leur ligne de texte ; les gagnantes
        // passent par le panneau de célébration, commun à tous les jeux.
        if (s.bj.payout == 0) {
            drawText(g, outcomeText(s.bj.outcome), kScreenW / 2, kActionsY + 4,
                     A(outcomeColor(s.bj.outcome)), 2, Align::Center);
        }
    } else if (g_demo) {
        drawText(g, "DEMO", kScreenW / 2, kActionsY + 4, kGrayLight, 2,
                 Align::Center);
    } else {
        drawText(g, "PRESS SPACE TO DEAL", kScreenW / 2, kActionsY + 4, P::cyan, 2,
                 Align::Center);
    }

    if (s.bj.phase == core::BjPhase::Settle && s.bj.payout > 0) {
        Celebration c;
        // Le blackjack paie 3:2 : c'est le sommet du jeu, il mérite le
        // palier le plus haut. Une victoire simple reste un palier moyen.
        c.tier = s.bj.outcome == core::BjOutcome::PlayerBlackjack
                     ? core::Tier::Big
                     : (s.bj.outcome == core::BjOutcome::Push ? core::Tier::Small
                                                              : core::Tier::Mid);
        c.payout = s.bj.payout;
        c.progress = core::celebrateProgress(core::Phase::Celebrate, c.tier,
                                             s.phaseT0, now);
        c.counted = core::countedPayout(c.payout, c.progress);
        c.demo = g_demo;
        c.label = outcomeText(s.bj.outcome);
        drawCelebration(g, c, now);
    }
}

}  // namespace ui
