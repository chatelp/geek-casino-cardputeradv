#include "menus.h"

#include "layout.h"
#include "painter.h"
#include "palette.h"
#include "bj_screen.h"
#include "slot_screen.h"
#include "symbols.h"
#include "video_screen.h"

namespace ui {

using namespace layout;
namespace P = pal;

namespace {

bool blink(uint32_t now, uint32_t periodMs) {
    return ((now / (periodMs / 2)) & 1u) != 0;
}

void drawHeader(lgfx::LGFX_Sprite& g, const char* title, uint16_t color) {
    g.fillRect(0, 0, kScreenW, 24, P::ink800);
    g.fillRect(0, 23, kScreenW, 1, color);
    drawText(g, title, 8, 5, color, 2);
}

void drawHint(lgfx::LGFX_Sprite& g, const char* hint) {
    g.fillRect(0, kScreenH - 13, kScreenW, 13, P::ink800);
    g.fillRect(0, kScreenH - 13, kScreenW, 1, P::ink600);
    drawText(g, hint, kScreenW / 2, kScreenH - 10, P::steel300, 1, Align::Center);
}

// ------------------------------------------------------------------ accueil
void drawLobby(lgfx::LGFX_Sprite& g, const core::App& app) {
    g.fillScreen(P::ink900);
    drawHeader(g, "GEEK CASINO", P::magenta);

    const core::Player* p = app.roster.count
            ? &app.roster.players[app.roster.current] : nullptr;
    if (p) {
        // Joueur courant + solde, à droite du titre.
        const int nw = textWidth(p->name, 1);
        drawText(g, p->name, kScreenW - 8 - nw, 3, P::cyan, 1, Align::Left);
        drawIcon(g, ICON_COIN, kScreenW - 76, 10);
        drawNumber(g, app.econ.credits, kScreenW - 60, 11, P::yellow, 1);
    }

    struct Entry { const char* name; const char* sub; uint8_t sym; };
    static const Entry entries[3] = {
        {"SLOTS", "3 REELS 1 LINE", core::SYM_INVADER},
        {"VIDEO SLOT", "5 REELS 5 LINES", core::SYM_CRT},
        {"BLACKJACK", "3:2 DEALER S17", core::SYM_D20},
    };
    for (int i = 0; i < 3; ++i) {
        const Entry& e = entries[i];
        const bool sel = app.lobbyIndex == i;
        const int y = 28 + i * 27;
        g.fillRect(6, y, kScreenW - 12, 26, sel ? P::ink700 : P::ink800);
        if (sel) {
            drawFrame(g, 6, y, kScreenW - 12, 26, P::cyan, 1);
            g.fillRect(6, y, 3, 26, P::cyan);
        }
        drawSymbol(g, e.sym, 14, y + 5, 1);
        drawText(g, e.name, 38, y + 2, sel ? P::white : P::steel300, 2);
        drawText(g, e.sub, 38, y + 18, sel ? P::cyan : P::ink600, 1);
    }

    // Jackpot courant : gain des 3 invaders à la mise en cours.
    g.fillRect(0, 110, kScreenW, 12, P::ink800);
    g.fillRect(0, 110, kScreenW, 1, P::greenDk);
    const int32_t jack = app.lobbyIndex == 1
        ? static_cast<int32_t>(app.video.pay->pay[core::kJackpotSymbol][5] *
                               core::bet(app.econ))
        : static_cast<int32_t>(app.game.machine.pay->pay[core::kJackpotSymbol][3] *
                               core::bet(app.econ));
    const int w = 16 + 4 + textWidth("JACKPOT ", 1) + numberWidth(jack, 1);
    const int x0 = (kScreenW - w) / 2;
    drawSymbol(g, core::SYM_INVADER, x0, 112, 1);
    drawText(g, "JACKPOT", x0 + 20, 113, P::green, 1);
    drawNumber(g, jack, x0 + 20 + textWidth("JACKPOT ", 1), 113, P::white, 1);

    drawHint(g, "H HELP  S SETTINGS  L BOARD");
}

// -------------------------------------------------------------- saisie du nom
void drawNameEntry(lgfx::LGFX_Sprite& g, const core::App& app, uint32_t now) {
    g.fillScreen(P::ink900);
    drawHeader(g, "WHO IS PLAYING?", P::cyan);

    // Huit cases, le curseur clignote sur la prochaine.
    const int cw = 22, gap = 4;
    const int x0 = (kScreenW - 8 * cw - 7 * gap) / 2;
    const int y = 52;
    for (int i = 0; i < core::kNameMax; ++i) {
        const int x = x0 + i * (cw + gap);
        g.fillRect(x, y, cw, 26, P::ink800);
        drawFrame(g, x, y, cw, 26, i == app.nameEntry.len ? P::cyan : P::ink600, 1);
        if (i < app.nameEntry.len) {
            const char s[2] = {app.nameEntry.buf[i], '\0'};
            drawText(g, s, x + cw / 2, y + 6, P::white, 2, Align::Center);
        } else if (i == app.nameEntry.len && blink(now, 700)) {
            g.fillRect(x + 4, y + 20, cw - 8, 2, P::cyan);
        }
    }

    if (app.nameEntry.rosterFull) {
        drawText(g, "BOARD FULL - 8 PLAYERS MAX", kScreenW / 2, 92, P::red, 1,
                 Align::Center);
    } else {
        drawText(g, "A-Z 0-9", kScreenW / 2, 92, P::steel300, 1, Align::Center);
    }
    drawHint(g, app.roster.count ? "ENTER OK  DEL ERASE  ESC CANCEL"
                                 : "ENTER OK  DEL ERASE");
}

// ------------------------------------------------------------------ aide slot
void drawSlotHelp(lgfx::LGFX_Sprite& g, const core::App& app) {
    g.fillScreen(P::ink900);
    drawHeader(g, "SLOTS - PAYTABLE", P::cyan);

    // 8 rangs sur deux colonnes : [geek] [classique] xN. Les deux habillages
    // côte à côte SONT la correspondance — pas besoin de mots.
    const core::Paytable& pt = *app.game.machine.pay;
    for (int i = 0; i < core::kSymbolCount; ++i) {
        const int col = i / 4, row = i % 4;
        const int x = 14 + col * 116;
        const int y = 28 + row * 19;
        drawSymbol(g, static_cast<uint8_t>(i), x, y + 1, 1);
        drawSymbol(g, static_cast<uint8_t>(i), x + 20, y + 1, 1, 0, /*classic=*/true);
        drawText(g, "x", x + 42, y + 3, P::steel300, 1);
        drawNumber(g, pt.pay[i][3], x + 50, y + 2, i == core::kJackpotSymbol
                   ? P::green : P::yellow, 2);
    }
    drawText(g, "ANY PAIR LEFT x2", kScreenW / 2, 106, P::steel300, 1, Align::Center);
    drawHint(g, "H OR ESC BACK");
}

void drawVideoHelp(lgfx::LGFX_Sprite& g, const core::App& app) {
    g.fillScreen(P::ink900);
    drawHeader(g, "VIDEO SLOT", P::cyan);
    const core::Paytable& pt = *app.video.pay;
    // Deux groupes de quatre rangs. Les nombres sont alignés à droite sur
    // des colonnes FIXES : à 15000, un alignement à gauche se chevauche.
    constexpr int kGroupX[2] = {2, 122};
    constexpr int kNumX[3] = {52, 84, 116};
    for (int col = 0; col < 2; ++col) {
        for (int k = 0; k < 3; ++k) {
            drawText(g, k == 0 ? "3" : (k == 1 ? "4" : "5"),
                     kGroupX[col] + kNumX[k], 26, P::steel500, 1, Align::Right);
        }
    }
    for (int i = 0; i < core::kSymbolCount; ++i) {
        const int col = i / 4, row = i % 4;
        const int x = kGroupX[col], y = 33 + row * 17;
        drawSymbol(g, static_cast<uint8_t>(i), x, y, 1, 0,
                   app.settings.slotSkin != 0);
        for (int k = 0; k < 3; ++k) {
            drawNumber(g, pt.pay[i][3 + k], x + kNumX[k], y + 5,
                       i == core::kJackpotSymbol ? P::green : P::yellow, 1,
                       Align::Right);
        }
    }
    drawText(g, "5 LINES - BET IS PER LINE", kScreenW / 2, 104, P::steel300, 1,
             Align::Center);
    drawHint(g, "H OR ESC BACK");
}

void drawBjHelp(lgfx::LGFX_Sprite& g, const core::App& app) {
    g.fillScreen(P::ink900);
    drawHeader(g, "BLACKJACK", P::cyan);
    // Chaque ligne tient en 23 caractères : au-delà, elle passerait sous
    // les cartes d'exemple posées à droite.
    static const char* kRules[] = {
        "BEAT THE DEALER TO 21",
        "ACE IS 1 OR 11",
        "FACES ARE WORTH 10",
        "BLACKJACK PAYS 3:2",
        "DEALER DRAWS TO 17",
        "DOUBLE ON FIRST 2",
    };
    for (int i = 0; i < 6; ++i) {
        g.fillRect(10, 30 + i * 13, 3, 3, i == 2 ? P::green : P::cyan);
        drawText(g, kRules[i], 18, 29 + i * 13, P::steel300, 1);
    }
    // Deux cartes d'exemple : le blackjack lui-même.
    drawCard(g, core::Card{1, SUIT_SPADE}, 168, 34, false);
    drawCard(g, core::Card{13, SUIT_HEART}, 196, 34, false);
    drawText(g, "3:2", 196, 80, P::green, 2, Align::Center);
    drawHint(g, "H OR ESC BACK");
}

// ------------------------------------------------------------------- réglages
void drawRow(lgfx::LGFX_Sprite& g, int y, bool sel, const char* label) {
    g.fillRect(6, y, kScreenW - 12, 20, sel ? P::ink700 : P::ink800);
    if (sel) {
        drawFrame(g, 6, y, kScreenW - 12, 20, P::cyan, 1);
        g.fillRect(6, y, 3, 20, P::cyan);
    }
    drawText(g, label, 16, y + 3, sel ? P::white : P::steel300, 2);
}

void drawGlobalSettings(lgfx::LGFX_Sprite& g, const core::App& app) {
    g.fillScreen(P::ink900);
    drawHeader(g, "SETTINGS", P::yellow);

    const int y0 = 28, dy = 22;
    drawRow(g, y0 + 0 * dy, app.menuIndex == 0, "SOUND");
    drawText(g, app.settings.muted ? "OFF" : "ON", kScreenW - 16, y0 + 3,
             app.settings.muted ? P::red : P::green, 2, Align::Right);

    drawRow(g, y0 + 1 * dy, app.menuIndex == 1, "VOLUME");
    for (int i = 0; i < 3; ++i) {  // trois barres de niveau
        const bool on = app.settings.volume > i;
        g.fillRect(kScreenW - 52 + i * 13, y0 + dy + 4 + (2 - i) * 4, 9,
                   4 + i * 4, on ? P::cyan : P::ink600);
    }

    drawRow(g, y0 + 2 * dy, app.menuIndex == 2, "PLAYER");
    {
        const core::Player* p = app.roster.count
            ? &app.roster.players[app.roster.current] : nullptr;
        drawText(g, p ? p->name : "-", kScreenW - 16, y0 + 2 * dy + 3, P::cyan, 2,
                 Align::Right);
    }

    drawRow(g, y0 + 3 * dy, app.menuIndex == 3, "RESET BOARD");
    if (app.resetArmed) {
        drawText(g, "SURE?", kScreenW - 16, y0 + 3 * dy + 3, P::red, 2, Align::Right);
    }

    drawHint(g, app.menuIndex == 2 ? "</> SWITCH  ENTER NEW PLAYER"
             : app.menuIndex == 3 ? "ENTER TWICE TO WIPE ALL"
                                  : "</> CHANGE  S OR ESC BACK");
}

void drawBjSettings(lgfx::LGFX_Sprite& g, const core::App& app) {
    g.fillScreen(P::ink900);
    drawHeader(g, "BLACKJACK SETTINGS", P::yellow);
    drawRow(g, 34, true, "HINTS");
    drawText(g, app.bj.hintsOn ? "ON" : "OFF", kScreenW - 16, 37,
             app.bj.hintsOn ? P::green : P::steel500, 2, Align::Right);
    drawText(g, "MARKS THE BASIC-STRATEGY MOVE", kScreenW / 2, 66, P::steel300, 1,
             Align::Center);
    drawText(g, "WITH A GREEN DOT - IT ADVISES,", kScreenW / 2, 78, P::steel300, 1,
             Align::Center);
    drawText(g, "IT NEVER PLAYS FOR YOU", kScreenW / 2, 90, P::steel300, 1,
             Align::Center);
    drawHint(g, "</> CHANGE  S OR ESC BACK");
}

void drawSlotSettings(lgfx::LGFX_Sprite& g, const core::App& app, bool video) {
    g.fillScreen(P::ink900);
    drawHeader(g, video ? "VIDEO SETTINGS" : "SLOTS SETTINGS", P::yellow);

    drawRow(g, 32, true, "GLYPHS");
    const bool classic = app.settings.slotSkin != 0;
    drawText(g, classic ? "CLASSIC" : "GEEK", kScreenW - 16, 35, P::cyan, 2,
             Align::Right);

    // Aperçu immédiat : les trois premiers symboles dans l'habillage choisi.
    for (int i = 0; i < 3; ++i) {
        g.fillRect(52 + i * 52, 62, 40, 40, P::ink800);
        drawFrame(g, 52 + i * 52, 62, 40, 40, P::ink600, 1);
        drawSymbol(g, static_cast<uint8_t>(i + 5), 56 + i * 52, 66, 2, 0, classic);
    }
    drawHint(g, "</> CHANGE  S OR ESC BACK");
}

// ------------------------------------------------------------------ classement
void drawLeaderboard(lgfx::LGFX_Sprite& g, const core::App& app) {
    g.fillScreen(P::ink900);
    drawHeader(g, "LEADERBOARD", P::green);

    if (app.roster.count == 0) {
        drawText(g, "NO PLAYERS YET", kScreenW / 2, 60, P::steel300, 2, Align::Center);
    } else {
        uint8_t order[core::kMaxPlayers];
        core::rankPlayers(app.roster, order);
        // 8 lignes de 11 px : tout le monde tient, le courant est surligné.
        for (uint8_t i = 0; i < app.roster.count; ++i) {
            const core::Player& p = app.roster.players[order[i]];
            const bool cur = order[i] == app.roster.current;
            const int y = 28 + i * 11;
            if (cur) g.fillRect(4, y - 1, kScreenW - 8, 11, P::ink700);
            drawNumber(g, i + 1, 14, y, i == 0 ? P::yellow : P::steel300, 1);
            drawText(g, p.name, 26, y, cur ? P::cyan : P::white, 1);
            drawIcon(g, ICON_COIN, 96, y - 1);
            drawNumber(g, p.credits, 110, y, P::yellow, 1);
            drawText(g, "BEST", 158, y, P::steel500, 1);
            drawNumber(g, static_cast<int32_t>(p.bestWin), 186, y, P::green, 1);
        }
    }
    drawHint(g, "L OR ESC BACK");
}

}  // namespace

void drawApp(lgfx::LGFX_Sprite& g, const core::App& app, uint32_t now) {
    switch (app.screen) {
        case core::AppScreen::NameEntry: drawNameEntry(g, app, now); break;
        case core::AppScreen::Lobby: drawLobby(g, app); break;
        case core::AppScreen::Slot:
            drawSlotScreen(g, app.game, now, app.settings.slotSkin != 0);
            break;
        case core::AppScreen::SlotHelp: drawSlotHelp(g, app); break;
        case core::AppScreen::SlotSettings: drawSlotSettings(g, app, false); break;
        case core::AppScreen::Video:
            drawVideoScreen(g, app.video, now, app.settings.slotSkin != 0);
            break;
        case core::AppScreen::VideoHelp: drawVideoHelp(g, app); break;
        case core::AppScreen::VideoSettings: drawSlotSettings(g, app, true); break;
        case core::AppScreen::Blackjack: drawBjScreen(g, app.bj, now); break;
        case core::AppScreen::BjHelp: drawBjHelp(g, app); break;
        case core::AppScreen::BjSettings: drawBjSettings(g, app); break;
        case core::AppScreen::GlobalSettings: drawGlobalSettings(g, app); break;
        case core::AppScreen::Leaderboard: drawLeaderboard(g, app); break;
    }
}

}  // namespace ui
