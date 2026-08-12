#include "slot_screen.h"

#include <cmath>

#include "celebration.h"
#include "layout.h"
#include "painter.h"
#include "palette.h"

namespace ui {

using namespace layout;
namespace P = pal;

namespace {

// Mode démo : niveaux de gris plutôt qu'une teinte plate. Trois nuances —
// les glyphes passent par kSymbolPaletteGray (mappage par luminance), le
// chrome par ces deux helpers. Le dessin garde son volume, la couleur part.
bool g_demo = false;
// GA : accents vifs → gris clair. GD : éléments sourds → gris moyen.
uint16_t A(uint16_t accent) { return g_demo ? kGrayLight : accent; }
uint16_t D(uint16_t dimmed) { return g_demo ? kGrayMid : dimmed; }

// Clignotement déterministe : même instant → même image. Indispensable pour
// que --shot et --frames produisent des captures reproductibles.
bool blink(uint32_t now, uint32_t periodMs) {
    return ((now / (periodMs / 2)) & 1u) != 0;
}

// Pistes hors cabinet : marge gauche ET droite. Le fond dit « vous êtes
// dans une machine », pas « rien à afficher ici ». Elles restent en ink700
// sur ink900 — présentes, jamais concurrentes du contenu.
void drawTraces(lgfx::LGFX_Sprite& g) {
    static const int16_t kSeg[][4] = {
        // marge gauche
        {4, 24, 2, 44},  {4, 66, 12, 2}, {14, 68, 2, 24}, {9, 30, 2, 24},
        {9, 30, 8, 2},   {4, 94, 2, 16}, {4, 108, 10, 2}, {16, 34, 2, 30},
        {12, 100, 2, 12},
        // marge droite, autour du levier — elles passent derrière lui, ce
        // qui est exactement ce que fait une piste sous un composant.
        {234, 26, 2, 40}, {222, 24, 14, 2}, {228, 66, 2, 26},
        {216, 90, 20, 2}, {234, 92, 2, 18}, {210, 30, 2, 22},
    };
    for (auto& s : kSeg) g.fillRect(s[0], s[1], s[2], s[3], P::ink700);
    static const int16_t kVia[][2] = {
        {3, 65}, {13, 91}, {15, 29}, {3, 107}, {11, 111},
        {233, 65}, {227, 89}, {233, 109}, {209, 29},
    };
    for (auto& v : kVia) g.fillRect(v[0], v[1], 4, 4, P::ink600);
}

// Pistes SUR la carte, autour des hublots. Le cabinet est le circuit :
// c'est ici qu'elles ont le plus de sens, et la place y est libre.
void drawBoardTraces(lgfx::LGFX_Sprite& g) {
    const int left = kCabX + 4, right = kCabX + kCabW - 6;
    const int top = kLampY + kLampSize + 2, bot = kWinY + kWinH;
    // Colonnes latérales, dans les 12 px de marge de part et d'autre.
    g.fillRect(left + 2, top, 2, bot - top - 2, P::ink700);
    g.fillRect(right - 2, top, 2, bot - top - 2, P::ink700);
    g.fillRect(left + 2, top, 8, 2, P::ink700);
    g.fillRect(right - 8, top, 8, 2, P::ink700);
    g.fillRect(left + 2, kPaylineY, 10, 2, P::ink700);
    g.fillRect(right - 10, kPaylineY, 10, 2, P::ink700);
    g.fillRect(left + 1, top - 2, 4, 4, P::ink600);
    g.fillRect(right - 4, top - 2, 4, 4, P::ink600);
    g.fillRect(left + 1, bot - 6, 4, 4, P::ink600);
    g.fillRect(right - 4, bot - 6, 4, 4, P::ink600);
    // Bus horizontal sous la rangée de lampes, entre les trous de fixation.
    g.fillRect(kCabX + 12, top, kCabW - 24, 1, P::ink700);
}

void drawHud(lgfx::LGFX_Sprite& g, const core::Game& game) {
    const core::Economy& e = game.machine.econ;
    g.fillRect(0, 0, kScreenW, kHudH, P::ink800);
    g.fillRect(0, kHudH - 1, kScreenW, 1, P::ink600);
    drawIcon(g, ICON_COIN, 6, 3);
    const bool low = e.credits < core::kBetLadder[0] * 10;
    drawNumber(g, e.credits, 22, 4, A(low ? P::red : P::yellow), 2);
    const int bw = numberWidth(core::bet(e), 2);
    drawNumber(g, core::bet(e), kScreenW - 10, 4, A(P::cyan), 2, Align::Right);
    drawText(g, "BET", kScreenW - 13 - bw - 9, 4, P::steel300, 2, Align::Right);
    // Réglable seulement à l'arrêt : les chevrons disparaissent pendant la
    // rotation, ce qui dit la règle sans un mot.
    const bool idle = game.phase != core::Phase::Spinning && !g_demo;
    drawBetArrows(g, kScreenW - 13 - bw - 9 - 10, kScreenW - 8, 5, P::steel500,
                  idle && e.betIndex > 0,
                  idle && e.betIndex + 1 < core::kBetSteps);
}

// Le bandeau de lampes est le canal d'expression le moins coûteux de
// l'écran : chenillard en rotation, tout allumé au gain, éteint à sec.
bool lampOn(const core::Game& game, uint32_t now, int i) {
    switch (game.phase) {
        case core::Phase::Spinning:
            return ((i + static_cast<int>(now / 90)) % 3) == 0;
        case core::Phase::Celebrate:
            return blink(now, 200) || game.tier >= core::Tier::Big;
        case core::Phase::Bailout:
            return false;
        default:
            return (i % 2) == 0;
    }
}

void drawCabinet(lgfx::LGFX_Sprite& g, const core::Game& game, uint32_t now) {
    uint16_t frame = D(P::cyanDk);
    if (game.phase == core::Phase::Celebrate) {
        frame = A(game.tier >= core::Tier::Big ? P::yellow : P::cyan);
    } else if (game.phase == core::Phase::Bailout) {
        frame = P::ink600;
    }
    g.fillRect(kCabX, kCabY, kCabW, kCabH, P::ink800);
    drawBoardTraces(g);
    drawFrame(g, kCabX, kCabY, kCabW, kCabH, frame, 2);

    const int hx[2] = {kCabX + 3, kCabX + kCabW - 3 - kHole};
    const int hy[2] = {kCabY + 3, kCabY + kCabH - 3 - kHole};
    for (int a = 0; a < 2; ++a) {
        for (int b = 0; b < 2; ++b) {
            g.fillRect(hx[a], hy[b], kHole, kHole, P::steel500);
            g.fillRect(hx[a] + 2, hy[b] + 2, kHole - 4, kHole - 4, P::ink900);
        }
    }
    for (int i = 0; i < kLampCount; ++i) {
        g.fillRect(kLampX0 + i * kLampStep, kLampY, kLampSize, kLampSize,
                   lampOn(game, now, i) ? A(P::yellow) : P::ink600);
    }
    const int bot = kWinY + kWinH;
    for (int i = 0; i < 3; ++i) {
        const int cx = winX(i) + kWinW / 2;
        g.fillRect(cx - 1, bot + 1, 2, 4, P::ink600);
        g.fillRect(cx - 2, bot + 5, 4, 3, P::steel500);
    }
    g.fillRect(kCabX + 8, bot + 6, kCabW - 16, 1, P::ink600);
}

// Au-delà de cette vitesse (symboles par image), le rouleau passe en flou.
constexpr float kBlurThreshold = 1.2f;
// Le flou défile bien plus lentement que le rouleau réel. C'est un mensonge
// assumé : à la vitesse vraie, l'image se réduirait à du bruit. L'œil lit
// « très vite » et n'a aucun moyen de compter les symboles.
constexpr float kBlurSlowdown = 0.22f;

// Flou de vitesse : le glyphe lui-même, étiré en traînées verticales.
// Quatre rangées de l'art sont échantillonnées et chacune s'étire sur un
// quart de symbole : les couleurs et la silhouette restent celles du
// glyphe qui passe, mais l'œil ne peut plus le lire — il lit « vite ».
void drawBlurredReel(lgfx::LGFX_Sprite& g, const core::ReelSet& rs, uint8_t r,
                     float pos, bool classic, uint16_t tint) {
    const float apparent = pos * kBlurSlowdown;
    const float base = std::floor(apparent);
    const int shift = static_cast<int>((apparent - base) * kPitch);
    const int streakH = kPitch / 4;  // 4 traînées par symbole
    for (int k = -1; k <= 2; ++k) {
        const uint8_t sym = core::symbolAt(rs, r, static_cast<int32_t>(base) + k);
        const uint8_t* art = classic ? kSymbolsClassic[sym] : kSymbols[sym];
        const int top = kSymY + k * kPitch - shift;
        for (int band = 0; band < 4; ++band) {
            const int row = 2 + band * 4;  // rangées 2, 6, 10, 14 de l'art
            const int y = top + band * streakH;
            int c = 0;
            while (c < kSymbolPx) {
                const uint8_t idx = art[row * kSymbolPx + c];
                if (idx == 0) { ++c; continue; }
                int run = 1;
                while (c + run < kSymbolPx && art[row * kSymbolPx + c + run] == idx) ++run;
                g.fillRect(symX(r) + c * kSymScale, y, run * kSymScale, streakH,
                           (tint ? kSymbolPaletteGray : kSymbolPalette)[idx]);
                c += run;
            }
        }
    }
}

void drawReels(lgfx::LGFX_Sprite& g, const core::Game& game, uint32_t now,
               bool classic) {
    const core::ReelSet& rs = *game.machine.reels;
    const bool celebrating = game.phase == core::Phase::Celebrate;
    const bool dead = game.phase == core::Phase::Bailout;

    for (uint8_t r = 0; r < rs.reels; ++r) {
        g.fillRect(winX(r), kWinY, kWinW, kWinH, P::ink900);
        drawFrame(g, winX(r) - 1, kWinY - 1, kWinW + 2, kWinH + 2, P::ink600, 1);

        const float p = core::reelDisplayPos(game, r, now);
        // Vitesse réelle, en symboles par image. Au-delà du seuil, montrer
        // les glyphes ne montre pas de la vitesse : ça scintille.
        const float speed =
            p - core::reelDisplayPos(game, r, now > core::kFrameMs ? now - core::kFrameMs : 0);

        // Découpe au hublot : un symbole qui défile déborde forcément.
        g.setClipRect(winX(r), kWinY, kWinW, kWinH);
        if (speed > kBlurThreshold) {
            drawBlurredReel(g, rs, r, p, classic, g_demo ? 1 : 0);
        } else {
            const float base = std::floor(p);
            const int shift = static_cast<int>((p - base) * kPitch);
            for (int k = -2; k <= 2; ++k) {
                const uint8_t sym =
                    core::symbolAt(rs, r, static_cast<int32_t>(base) + k);
                if (g_demo) {
                    drawSymbolGray(g, sym, symX(r), kSymY + k * kPitch - shift,
                                   kSymScale, classic);
                } else {
                    drawSymbol(g, sym, symX(r), kSymY + k * kPitch - shift,
                               kSymScale, dead ? P::ink600 : 0, classic);
                }
            }
        }
        g.clearClipRect();

        if (celebrating && blink(now, 260)) {
            drawFrame(g, winX(r) - 1, kWinY - 1, kWinW + 2, kWinH + 2, A(P::white), 1);
        }
    }
}

void drawPayline(lgfx::LGFX_Sprite& g, const core::Game& game) {
    uint16_t c = D(P::magentaDk);
    if (game.phase == core::Phase::Celebrate) c = A(P::magenta);
    else if (game.phase == core::Phase::Bailout) c = P::ink600;
    for (int i = 0; i < 3; ++i) g.fillRect(winX(i), kPaylineY, kWinW, 1, c);
    for (int k = 0; k < 4; ++k) {
        g.fillRect(winX(0) - 3 - k, kPaylineY - k, 1, 1 + 2 * k, c);
        g.fillRect(winX(2) + kWinW + 2 + k, kPaylineY - k, 1, 1 + 2 * k, c);
    }
}

// Le levier traduit le geste : il plonge au lancement et remonte doucement.
void drawLever(lgfx::LGFX_Sprite& g, const core::Game& game, uint32_t now) {
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
    g.fillRect(kLeverCx - 11, kLeverBaseY + 3, 22, 9, P::steel500);
    drawFrame(g, kLeverCx - 11, kLeverBaseY + 3, 22, 9, P::ink900, 1);
    g.fillRect(kLeverCx - 7, kLeverBaseY + 6, 14, 3, P::ink800);
    if (pull > 0.5f) {
        for (int k = 0; k < 3; ++k) {
            g.fillRect(kLeverCx - 12 - k * 3, top + 4, 2, 2, A(P::yellow));
            g.fillRect(kLeverCx + 11 + k * 3, top + 4, 2, 2, A(P::yellow));
        }
    }
}

// Gerbes depuis les angles du cabinet, jamais isolées au milieu d'un bord
// où elles se liraient comme des pixels morts.
void drawSparks(lgfx::LGFX_Sprite& g, const core::Game& game, uint32_t now) {
    if (g_demo) return;
    if (game.phase != core::Phase::Celebrate || game.tier < core::Tier::Mid) return;
    const int n = game.tier >= core::Tier::Big ? 10 : 4;
    static const int16_t kS[][4] = {
        {16, 13, 3, 0}, {204, 13, 3, 0}, {15, 114, 2, 1}, {203, 114, 2, 1},
        {8, 21, 2, 1},  {212, 21, 2, 1}, {24, 6, 2, 2},   {196, 6, 2, 2},
        {6, 107, 2, 0}, {212, 107, 2, 0},
    };
    const uint16_t cols[3] = {P::yellow, P::orange, P::white};
    const uint32_t step = now / 110;
    for (int i = 0; i < n; ++i) {
        if (((i + step) & 3u) == 0) continue;  // scintillement
        g.fillRect(kS[i][0], kS[i][1], kS[i][2], kS[i][2], cols[kS[i][3]]);
    }
}

void drawMessage(lgfx::LGFX_Sprite& g, const core::Game& game, uint32_t now) {
    g.fillRect(0, kMsgY, kScreenW, kScreenH - kMsgY, P::ink800);
    g.fillRect(0, kMsgY, kScreenW, 1, P::ink600);
    const int ty = kMsgY + (kScreenH - kMsgY - kFontH * 2) / 2;

    if (g_demo) {
        // Le mot + le monochrome : impossible de croire à une partie.
        drawText(g, "DEMO", kScreenW / 2, ty, kGrayLight, 2, Align::Center);
        return;
    }
    switch (game.phase) {
        case core::Phase::Idle:
            drawText(g, "SHAKE TO SPIN", kScreenW / 2, ty, P::cyan, 2, Align::Center);
            break;
        case core::Phase::Spinning:
            // Vingt pixels de haut sur toute la largeur ne servaient qu'à
            // écrire « SPINNING ». Le spectre, lui, raconte le tour : il
            // s'apaise à chaque rouleau qui se verrouille, et le dernier
            // rouleau le fait cogner.
            drawScope(g, 0, kMsgY + 2, kScreenW, kScreenH - kMsgY - 2,
                         core::scopeDriveOfReels(game.motion, core::kMvpReels, now),
                         now);
            break;
        case core::Phase::Celebrate:
            // Le montant vit dans le panneau de célébration ; ici le
            // spectre continue, à la hauteur du palier — un petit gain ne
            // doit pas faire le même bruit visuel qu'un jackpot (D-008).
            drawScope(g, 0, kMsgY + 2, kScreenW, kScreenH - kMsgY - 2,
                         core::scopeDriveOfWin(static_cast<uint8_t>(game.tier), now), now);
            break;
        case core::Phase::Bailout:
            drawText(g, "THE HOUSE REFILLS", kScreenW / 2, kMsgY + 3, P::green, 1,
                     Align::Center);
            drawText(g, "+500", kScreenW / 2, kMsgY + 12, P::white, 2, Align::Center);
            break;
    }
}

// Jackpot : le cabinet disparaît. C'est le seul état qui casse la mise en
// page — c'est précisément ce qui le rend énorme.
void drawJackpot(lgfx::LGFX_Sprite& g, const core::Game& game, uint32_t now) {
    g.fillScreen(P::ink900);
    static const int16_t kInv[][2] = {{6, 8}, {206, 14}, {30, 96}, {186, 104}, {110, 4}};
    for (auto& v : kInv) {
        if (g_demo) drawSymbolGray(g, core::SYM_INVADER, v[0], v[1], 2);
        else drawSymbol(g, core::SYM_INVADER, v[0], v[1], 2, P::greenDk);
    }
    g.fillRect(0, 34, kScreenW, 46, P::ink800);
    g.fillRect(0, 34, kScreenW, 2, A(P::green));
    g.fillRect(0, 78, kScreenW, 2, A(P::green));
    if (blink(now, 400)) {
        drawText(g, "JACKPOT", kScreenW / 2, 40, A(P::green), 3, Align::Center);
    }
    drawNumber(g, static_cast<int32_t>(game.outcome.payout), kScreenW / 2, 64,
               P::white, 2, Align::Center);
    for (int i = 0; i < 3; ++i) {
        if (g_demo) drawSymbolGray(g, core::SYM_INVADER, 42 + i * 54, 88, 2);
        else drawSymbol(g, core::SYM_INVADER, 42 + i * 54, 88, 2, P::green);
    }
    const uint32_t step = now / 120;
    for (int x = 0; x < kScreenW; x += 8) {
        const bool a = (((x / 8) + step) & 1u) == 0;
        g.fillRect(x, 128, 4, 4, a ? A(P::yellow) : A(P::magenta));
    }
}

}  // namespace

void drawSlotScreen(lgfx::LGFX_Sprite& g, const core::Game& game, uint32_t now,
                    bool classic) {
    g_demo = game.attract;
    g.fillScreen(P::ink900);
    drawTraces(g);
    drawHud(g, game);
    drawCabinet(g, game, now);
    drawReels(g, game, now, classic);
    drawPayline(g, game);
    drawLever(g, game, now);
    drawSparks(g, game, now);
    drawMessage(g, game, now);

    if (game.phase == core::Phase::Celebrate) {
        Celebration c;
        c.tier = game.tier;
        c.payout = game.outcome.payout;
        c.multiplier = game.outcome.win.multiplier;
        c.progress = core::celebrateProgress(game.phase, game.tier, game.phaseT0, now);
        c.counted = core::countedPayout(c.payout, c.progress);
        c.demo = g_demo;
        drawCelebration(g, c, now);
    }
}

}  // namespace ui
