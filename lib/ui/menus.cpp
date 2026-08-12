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

// Pagination : des points pleins/vides, plus une flèche tant qu'il reste
// une page. Sans ce repère, personne ne devine qu'il y a une suite.
void drawPager(lgfx::LGFX_Sprite& g, uint8_t page, uint8_t count, uint32_t now) {
    if (count < 2) return;
    const int w = count * 7 - 3;
    const int x0 = kScreenW - 8 - w;
    for (uint8_t i = 0; i < count; ++i) {
        const int x = x0 + i * 7;
        if (i == page) g.fillRect(x, 9, 4, 4, P::cyan);
        else drawFrame(g, x, 9, 4, 4, P::steel500, 1);
    }
    if (page + 1 < count && blink(now, 900)) {
        // Chevron vers le bas, à droite du bandeau d'aide.
        for (int k = 0; k < 3; ++k) {
            g.fillRect(kScreenW - 12 + k, kScreenH - 11 + k, 1 + 2 * (2 - k), 1,
                       P::cyan);
        }
    }
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
    // Chaque jeu a sa mise : le jackpot annoncé doit être celui du jeu
    // sélectionné, pas d'une mise commune qui n'existe plus.
    const core::GameId gid = static_cast<core::GameId>(app.lobbyIndex);
    const uint16_t b = core::betOfGame(app, gid);
    const int32_t jack = gid == core::GameId::Video
        ? static_cast<int32_t>(app.video.pay->pay[core::kJackpotSymbol][5] * b)
        : static_cast<int32_t>(app.game.machine.pay->pay[core::kJackpotSymbol][3] * b);
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
void drawSlotHelp(lgfx::LGFX_Sprite& g, const core::App& app, uint32_t now) {
    g.fillScreen(P::ink900);
    const uint8_t pages = core::helpPageCount(core::AppScreen::SlotHelp);

    if (app.helpPage == 0) {
        drawHeader(g, "SLOTS - PAYTABLE", P::cyan);
        // Les deux habillages côte à côte SONT la correspondance.
        const core::Paytable& pt = *app.game.machine.pay;
        for (int i = 0; i < core::kSymbolCount; ++i) {
            const int col = i / 4, row = i % 4;
            const int x = 14 + col * 116;
            const int y = 28 + row * 19;
            drawSymbol(g, static_cast<uint8_t>(i), x, y + 1, 1);
            drawSymbol(g, static_cast<uint8_t>(i), x + 20, y + 1, 1, 0, true);
            drawText(g, "x", x + 42, y + 3, P::steel300, 1);
            drawNumber(g, pt.pay[i][3], x + 50, y + 2,
                       i == core::kJackpotSymbol ? P::green : P::yellow, 2);
        }
        drawText(g, "ANY PAIR LEFT x2", kScreenW / 2, 106, P::steel300, 1,
                 Align::Center);
    } else {
        drawHeader(g, "SLOTS - HOW TO PLAY", P::cyan);
        static const char* kRules[] = {
            "3 REELS, 1 PAYLINE - THE MIDDLE ROW",
            "COMBOS READ FROM THE LEFT ONLY",
            "3 ALIKE PAYS - 2 ALIKE PAYS x2",
            "SAME PAIR NOT ON THE LEFT PAYS 0",
            "</> CHANGE YOUR BET BETWEEN SPINS",
            "SHAKE THE DEVICE OR PRESS SPACE",
            "S SETTINGS - SWITCH GLYPH STYLE",
        };
        constexpr int kn = sizeof(kRules) / sizeof(kRules[0]);
        for (int i = 0; i < kn; ++i) {
            g.fillRect(10, 30 + i * 11, 3, 3, i == 5 ? P::magenta : P::cyan);
            drawText(g, kRules[i], 18, 29 + i * 11, P::steel300, 1);
        }
    }
    drawPager(g, app.helpPage, pages, now);
    drawHint(g, app.helpPage + 1 < pages ? "V MORE   H OR ESC BACK"
                                         : "^ BACK   H OR ESC CLOSE");
}

// Schéma d'une ligne de paiement : la grille 5x3 en miniature, avec la
// ligne tracée dessus. C'est la seule façon de montrer un chevron — cinq
// phrases ne le feraient pas comprendre.
void drawPaylineDiagram(lgfx::LGFX_Sprite& g, const core::Payline& pl, int x,
                        int y, uint16_t col) {
    constexpr int kCell = 7;
    for (int c = 0; c < core::kVideoReels; ++c) {
        for (int r = 0; r < core::kVideoRows; ++r) {
            const int cx = x + c * kCell, cy = y + r * kCell;
            drawFrame(g, cx, cy, kCell - 1, kCell - 1, P::ink600, 1);
        }
    }
    // Cellules de la ligne, puis les segments qui les relient.
    for (int c = 0; c < core::kVideoReels; ++c) {
        const int cx = x + c * kCell, cy = y + pl.row[c] * kCell;
        g.fillRect(cx + 1, cy + 1, kCell - 3, kCell - 3, col);
        if (c + 1 < core::kVideoReels) {
            const int ny = y + pl.row[c + 1] * kCell;
            const int top = cy < ny ? cy : ny;
            const int len = (cy < ny ? ny - cy : cy - ny) + 1;
            g.fillRect(cx + kCell - 2, top + kCell / 2 - 1, 2, len, col);
        }
    }
}

void drawVideoHelp(lgfx::LGFX_Sprite& g, const core::App& app, uint32_t now) {
    g.fillScreen(P::ink900);
    const uint8_t pages = core::helpPageCount(core::AppScreen::VideoHelp);

    if (app.helpPage == 0) {
        drawHeader(g, "VIDEO SLOT - PAYTABLE", P::cyan);
        const core::Paytable& pt = *app.video.pay;
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
        drawText(g, "PAYS FOR 3, 4 OR 5 FROM THE LEFT", kScreenW / 2, 104,
                 P::steel300, 1, Align::Center);
    } else if (app.helpPage == 1) {
        drawHeader(g, "VIDEO SLOT - 5 LINES", P::magenta);
        // Cinq schémas : trois en haut, deux en bas, tous à la même échelle.
        const core::Payline* lines = core::videoPaylines();
        static const char* kNames[core::kVideoLines] = {
            "MIDDLE", "TOP", "BOTTOM", "V DOWN", "V UP",
        };
        constexpr int kW = 35, kH = 21;
        for (int i = 0; i < core::kVideoLines; ++i) {
            const int col = i < 3 ? i : i - 3;
            const int rowY = i < 3 ? 30 : 74;
            const int count = i < 3 ? 3 : 2;
            const int x = (kScreenW - count * (kW + 14) + 14) / 2 + col * (kW + 14);
            drawPaylineDiagram(g, lines[i], x, rowY, P::magenta);
            drawText(g, kNames[i], x + kW / 2 - 1, rowY + kH + 2, P::steel300, 1,
                     Align::Center);
        }
        drawText(g, "EVERY LINE IS ALWAYS PLAYED", kScreenW / 2, 108,
                 P::cyan, 1, Align::Center);
    } else {
        drawHeader(g, "VIDEO SLOT - HOW TO PLAY", P::cyan);
        static const char* kRules[] = {
            "5 REELS, 3 ROWS, 5 PAYLINES",
            "YOUR BET IS PLACED ON EACH LINE",
            "SO ONE SPIN COSTS BET x5",
            "3 ALIKE FROM THE LEFT ALREADY PAYS",
            "EVERY WINNING LINE PAYS, THEY ADD UP",
            "5 INVADERS ON A LINE IS THE JACKPOT",
            "</> BET   SHAKE OR SPACE TO SPIN",
        };
        constexpr int kn = sizeof(kRules) / sizeof(kRules[0]);
        for (int i = 0; i < kn; ++i) {
            g.fillRect(10, 30 + i * 11, 3, 3, i == 5 ? P::green : P::cyan);
            drawText(g, kRules[i], 18, 29 + i * 11, P::steel300, 1);
        }
    }
    drawPager(g, app.helpPage, pages, now);
    drawHint(g, app.helpPage + 1 < pages ? "V MORE   H OR ESC BACK"
                                         : "^ BACK   H OR ESC CLOSE");
}

void drawBjHelp(lgfx::LGFX_Sprite& g, const core::App& app, uint32_t now) {
    g.fillScreen(P::ink900);
    const uint8_t pages = core::helpPageCount(core::AppScreen::BjHelp);

    if (app.helpPage == 0) {
        drawHeader(g, "BLACKJACK - RULES", P::cyan);
        static const char* kRules[] = {
            "BEAT THE DEALER TO 21",
            "ACE IS 1 OR 11",
            "FACES ARE WORTH 10",
            "BLACKJACK PAYS 3:2",
            "DEALER DRAWS TO 17",
            "TIE RETURNS YOUR BET",
        };
        constexpr int kn = sizeof(kRules) / sizeof(kRules[0]);
        for (int i = 0; i < kn; ++i) {
            g.fillRect(10, 30 + i * 12, 3, 3, i == 3 ? P::green : P::cyan);
            drawText(g, kRules[i], 18, 29 + i * 12, P::steel300, 1);
        }
        drawCard(g, core::Card{1, SUIT_SPADE}, 168, 34, false);
        drawCard(g, core::Card{13, SUIT_HEART}, 196, 34, false);
        drawText(g, "3:2", 196, 80, P::green, 2, Align::Center);
    } else {
        drawHeader(g, "BLACKJACK - ACTIONS", P::cyan);
        struct Act { const char* name; const char* what; };
        static const Act kActs[] = {
            {"HIT", "TAKE ONE MORE CARD"},
            {"STAND", "KEEP YOUR HAND, DEALER PLAYS"},
            {"DOUBLE", "DOUBLE THE BET, ONE CARD ONLY"},
        };
        for (int i = 0; i < 3; ++i) {
            const int y = 30 + i * 20;
            g.fillRect(10, y, 56, 15, P::ink700);
            drawFrame(g, 10, y, 56, 15, P::cyan, 1);
            drawText(g, kActs[i].name, 14, y + 3, P::white, 2);
            drawText(g, kActs[i].what, 72, y + 5, P::steel300, 1);
        }
        drawText(g, "</> PICK, ENTER CONFIRMS", 10, 94, P::cyan, 1);
        g.fillRect(10, 106, 3, 3, P::green);
        drawText(g, "HINTS ON: GREEN DOT MARKS THE", 18, 104, P::steel300, 1);
        drawText(g, "BASIC-STRATEGY MOVE", 18, 113, P::steel300, 1);
    }
    drawPager(g, app.helpPage, pages, now);
    drawHint(g, app.helpPage + 1 < pages ? "V MORE   H OR ESC BACK"
                                         : "^ BACK   H OR ESC CLOSE");
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
        case core::AppScreen::SlotHelp: drawSlotHelp(g, app, now); break;
        case core::AppScreen::SlotSettings: drawSlotSettings(g, app, false); break;
        case core::AppScreen::Video:
            drawVideoScreen(g, app.video, now, app.settings.slotSkin != 0);
            break;
        case core::AppScreen::VideoHelp: drawVideoHelp(g, app, now); break;
        case core::AppScreen::VideoSettings: drawSlotSettings(g, app, true); break;
        case core::AppScreen::Blackjack: drawBjScreen(g, app.bj, now); break;
        case core::AppScreen::BjHelp: drawBjHelp(g, app, now); break;
        case core::AppScreen::BjSettings: drawBjSettings(g, app); break;
        case core::AppScreen::GlobalSettings: drawGlobalSettings(g, app); break;
        case core::AppScreen::Leaderboard: drawLeaderboard(g, app); break;
    }
}

}  // namespace ui
