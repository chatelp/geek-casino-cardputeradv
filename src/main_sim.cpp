// Main simulateur macOS — LovyanGFX + SDL2 (Panel_sdl), fenêtre ×3.
//
//   (aucune option)      fenêtre interactive
//   --shot <dir>         une image déterministe, sans fenêtre
//   --frames <dir> <n>   n images déterministes, pour un GIF
//   --screens <dir>      une image par écran (accueil, aide, réglages...)
//
// Clavier : voir core/app.h. Le son n'existe pas au simulateur — c'est
// précisément ce que le sim juge mal, il s'écoute sur l'appareil.
#ifdef SIM_BUILD

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>

#include "app.h"
#include "hal_display.h"
#include "menus.h"
#include "rng.h"

namespace {

class SimDisplay : public lgfx::LGFX_Device {
    lgfx::Panel_sdl _panel;

    bool init_impl(bool /*use_reset*/, bool use_clear) override {
        return LGFX_Device::init_impl(false, use_clear);
    }

public:
    SimDisplay() {
        auto cfg = _panel.config();
        cfg.memory_width = hal::kScreenW;
        cfg.panel_width = hal::kScreenW;
        cfg.memory_height = hal::kScreenH;
        cfg.panel_height = hal::kScreenH;
        _panel.config(cfg);
        _panel.setScaling(3, 3);
        _panel.setWindowTitle("Geek Casino — sim");
        setPanel(&_panel);
    }
};

void requestQuit() {
    SDL_Event ev;
    ev.type = SDL_QUIT;
    SDL_PushEvent(&ev);
}

std::string g_outDir;
enum class Mode { Interactive, Shot, Frames, Screens };
Mode g_mode = Mode::Interactive;
int g_frameCount = 0;
constexpr uint32_t kFixedSeed = 0xCA51704Du;
constexpr char kSavePath[] = "geekcasino.sav";

// Écrit un BMP 24 bits. readRect renvoie du RGB565 aux octets inversés
// (piège documenté dans CLAUDE.md) : on corrige ici.
bool writeBmp(lgfx::LGFX_Sprite& g, const std::string& path) {
    const int w = g.width(), h = g.height();
    static uint16_t px[hal::kScreenW * hal::kScreenH];
    g.readRect(0, 0, w, h, px);

    const uint32_t rowBytes = ((w * 3 + 3) / 4) * 4;
    uint8_t hdr[54] = {'B', 'M'};
    auto put32 = [&](int off, uint32_t v) {
        hdr[off] = v; hdr[off + 1] = v >> 8; hdr[off + 2] = v >> 16; hdr[off + 3] = v >> 24;
    };
    put32(2, 54 + rowBytes * h); put32(10, 54); put32(14, 40);
    put32(18, static_cast<uint32_t>(w)); put32(22, static_cast<uint32_t>(h));
    hdr[26] = 1; hdr[28] = 24;
    put32(34, rowBytes * h);

    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(hdr, 1, 54, f);
    static uint8_t row[((hal::kScreenW * 3 + 3) / 4) * 4];
    for (int y = h - 1; y >= 0; --y) {
        std::memset(row, 0, sizeof(row));
        for (int x = 0; x < w; ++x) {
            const uint16_t s = px[y * w + x];
            const uint16_t c = static_cast<uint16_t>((s >> 8) | (s << 8));
            row[x * 3 + 0] = static_cast<uint8_t>(((c >> 0) & 0x1F) * 255 / 31);
            row[x * 3 + 1] = static_cast<uint8_t>(((c >> 5) & 0x3F) * 255 / 63);
            row[x * 3 + 2] = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);
        }
        std::fwrite(row, 1, rowBytes, f);
    }
    std::fclose(f);
    return true;
}

// ------------------------------------------------------------- persistance
void loadApp(core::App& a) {
    FILE* f = std::fopen(kSavePath, "rb");
    if (!f) return;
    core::SaveData s{};
    const bool ok = std::fread(&s, sizeof(s), 1, f) == 1;
    std::fclose(f);
    if (ok && core::applySave(s, a.roster, a.settings)) {
        // Bloc de mises facultatif : une sauvegarde d'avant cette
        // fonctionnalité se charge normalement, avec les mises par défaut.
        core::BetMemory b{};
        FILE* fb = std::fopen(kSavePath, "rb");
        if (fb) {
            std::fseek(fb, sizeof(s), SEEK_SET);
            if (std::fread(&b, sizeof(b), 1, fb) == 1 && core::betsValid(b)) {
                a.bets = b;
            }
            std::fclose(fb);
        }
        core::enterFromSave(a);
    }
    // Sauvegarde illisible : on repart sur la saisie du nom, sans casser
    // le fichier — il sera réécrit à la première partie.
}

void saveApp(core::App& a) {
    core::pullEconomy(a);
    core::syncPlayer(a.roster, a.econ, a.game.spins + a.video.spins + a.bj.hands, 0);
    core::storePlayerBets(a);
    const core::SaveData s = core::makeSave(a.roster, a.settings);
    const core::BetMemory b = core::makeBets(a.bets);
    FILE* f = std::fopen(kSavePath, "wb");
    if (!f) return;
    std::fwrite(&s, sizeof(s), 1, f);
    std::fwrite(&b, sizeof(b), 1, f);
    std::fclose(f);
    a.dirty = false;
}

// ----------------------------------------------------------------- captures
bool saveShot(lgfx::LGFX_Sprite& canvas, const char* name) {
    if (!writeBmp(canvas, g_outDir + "/" + name)) {
        std::fprintf(stderr, "capture : echec d'ecriture dans %s\n", g_outDir.c_str());
        return false;
    }
    return true;
}

int runCapture() {
    core::seedXorShift(kFixedSeed);
    lgfx::LGFX_Sprite canvas(nullptr);
    canvas.setColorDepth(16);
    canvas.createSprite(hal::kScreenW, hal::kScreenH);

    uint32_t now = 0;
    core::App app = core::newApp(now, core::xorShift32);
    core::addOrSwitchPlayer(app.roster, "PIXEL");
    core::enterFromSave(app);

    if (g_mode == Mode::Screens) {
        // Un état représentatif de chaque écran, toujours le même.
        core::addOrSwitchPlayer(app.roster, "MARIO");
        app.roster.players[1].credits = 720;
        app.roster.players[1].bestWin = 250;
        core::addOrSwitchPlayer(app.roster, "PIXEL");

        struct Shotdef { core::AppScreen sc; const char* name; };
        static const Shotdef defs[] = {
            {core::AppScreen::Lobby, "lobby.bmp"},
            {core::AppScreen::SlotHelp, "help.bmp"},
            {core::AppScreen::GlobalSettings, "settings.bmp"},
            {core::AppScreen::SlotSettings, "slot_settings.bmp"},
            {core::AppScreen::Leaderboard, "leaderboard.bmp"},
        };
        for (const auto& d : defs) {
            app.screen = d.sc;
            ui::drawApp(canvas, app, 400);
            if (!saveShot(canvas, d.name)) return 1;
        }
        // La saisie du nom, à moitié remplie, curseur visible.
        app.roster.count = 0;
        app.screen = core::AppScreen::NameEntry;
        core::feedNameChar(app, 'Z'); core::feedNameChar(app, 'O');
        core::feedNameChar(app, 'E');
        ui::drawApp(canvas, app, 400);
        if (!saveShot(canvas, "name_entry.bmp")) return 1;
        // Le jeu en habillage classique.
        core::addOrSwitchPlayer(app.roster, "PIXEL");
        app.settings.slotSkin = 1;
        app.screen = core::AppScreen::Slot;
        ui::drawApp(canvas, app, 400);
        if (!saveShot(canvas, "slot_classic.bmp")) return 1;
        // Les deux nouveaux jeux.
        app.settings.slotSkin = 0;
        app.screen = core::AppScreen::Video;
        uint32_t vt = 500;
        core::startVideoSpin(app.video, vt, core::xorShift32);
        for (int i = 0; i < 200; ++i) {
            vt += core::kFrameMs;
            core::updateVideoGame(app.video, vt, core::xorShift32);
            if (app.video.phase == core::Phase::Celebrate) break;
        }
        ui::drawApp(canvas, app, vt);
        if (!saveShot(canvas, "video.bmp")) return 1;
        app.screen = core::AppScreen::VideoHelp;
        ui::drawApp(canvas, app, vt);
        if (!saveShot(canvas, "video_help.bmp")) return 1;
        for (uint8_t pg = 1; pg < core::helpPageCount(core::AppScreen::VideoHelp); ++pg) {
            const_cast<core::App&>(app).helpPage = pg;
            ui::drawApp(canvas, app, vt);
            if (!saveShot(canvas, pg == 1 ? "video_lines.bmp" : "video_rules.bmp")) return 1;
        }
        const_cast<core::App&>(app).helpPage = 0;

        app.screen = core::AppScreen::Blackjack;
        app.bj.hintsOn = true;
        uint32_t bt = 500;
        core::bjStartHand(app.bj, bt, core::xorShift32);
        for (int i = 0; i < 60; ++i) {
            bt += core::kFrameMs;
            core::bjUpdate(app.bj, bt, core::xorShift32);
        }
        ui::drawApp(canvas, app, bt);
        if (!saveShot(canvas, "blackjack.bmp")) return 1;
        // Table vide : c'est là que les empreintes de composants et la
        // sérigraphie se voient, donc là que l'identité se lit le mieux.
        app.bj = core::newBjSession(0);
        ui::drawApp(canvas, app, 400);
        if (!saveShot(canvas, "bj_table.bmp")) return 1;
        core::bjStartHand(app.bj, bt, core::xorShift32);
        for (int i = 0; i < 60; ++i) { bt += core::kFrameMs; core::bjUpdate(app.bj, bt, core::xorShift32); }
        app.screen = core::AppScreen::BjHelp;
        ui::drawApp(canvas, app, bt);
        if (!saveShot(canvas, "bj_help.bmp")) return 1;
        app.screen = core::AppScreen::BjSettings;
        ui::drawApp(canvas, app, bt);
        if (!saveShot(canvas, "bj_settings.bmp")) return 1;

        // Une célébration à trois instants : ouverture, décompte, total.
        app.screen = core::AppScreen::Slot;
        app.settings.slotSkin = 0;
        app.game.attract = false;
        app.game.phase = core::Phase::Celebrate;
        app.game.tier = core::Tier::Big;
        app.game.outcome.payout = 1250;
        app.game.outcome.win.multiplier = 250;
        app.game.phaseT0 = 1000;
        const uint32_t dur = core::celebrateMs(core::Tier::Big);
        struct Moment { float f; const char* name; };
        static const Moment kMoments[] = {
            {0.08f, "celeb_open.bmp"}, {0.30f, "celeb_count.bmp"},
            {0.80f, "celeb_hold.bmp"},
        };
        for (const auto& m : kMoments) {
            ui::drawApp(canvas, app, 1000 + static_cast<uint32_t>(dur * m.f));
            if (!saveShot(canvas, m.name)) return 1;
        }
        // Séquence complète de la célébration, pour en juger le rythme.
        for (uint32_t t = 0; t <= dur + 300; t += core::kFrameMs) {
            char nm[32];
            std::snprintf(nm, sizeof(nm), "celeb_%04u.bmp", t / core::kFrameMs);
            ui::drawApp(canvas, app, 1000 + t);
            if (!saveShot(canvas, nm)) return 1;
        }

        // Le mode démo : monochrome gris, un tour gratuit en cours.
        app.screen = core::AppScreen::Slot;
        app.settings.slotSkin = 0;
        uint32_t t = 1000;
        core::startSpin(app.game, t, core::xorShift32, /*byPlayer=*/false);
        for (int i = 0; i < 40; ++i) { t += core::kFrameMs; core::updateGame(app.game, t, core::xorShift32); }
        ui::drawApp(canvas, app, t);
        if (!saveShot(canvas, "demo.bmp")) return 1;
        return 0;
    }

    app.screen = core::AppScreen::Slot;
    core::startSpin(app.game, now, core::xorShift32);
    for (int i = 0; i < g_frameCount; ++i) {
        core::tickApp(app, now, core::xorShift32);
        if (app.game.phase == core::Phase::Idle) {
            core::startSpin(app.game, now, core::xorShift32);
        }
        ui::drawApp(canvas, app, now);
        char name[32];
        if (g_mode == Mode::Shot) std::snprintf(name, sizeof(name), "shot.bmp");
        else std::snprintf(name, sizeof(name), "frame_%04d.bmp", i);
        if (!saveShot(canvas, name)) return 1;
        now += core::kFrameMs;
    }
    return 0;
}

// --------------------------------------------------------------- interactif
struct KeyEdge {
    bool prev[SDL_NUM_SCANCODES] = {false};

    bool pressed(const uint8_t* st, SDL_Scancode sc) {
        const bool down = st[sc] != 0;
        const bool edge = down && !prev[sc];
        prev[sc] = down;
        return edge;
    }
};

int simRun(bool* running) {
    core::seedXorShift(kFixedSeed);

    SimDisplay display;
    display.init();
    lgfx::LGFX_Sprite canvas(&display);
    canvas.setColorDepth(16);
    canvas.createSprite(hal::kScreenW, hal::kScreenH);

    const uint32_t t0 = SDL_GetTicks();
    core::App app = core::newApp(0, core::xorShift32);
    loadApp(app);
    KeyEdge edge;

    while (*running) {
        const uint32_t now = SDL_GetTicks() - t0;
        const uint8_t* st = SDL_GetKeyboardState(nullptr);
        const bool naming = app.screen == core::AppScreen::NameEntry;

        if (naming) {
            for (int sc = SDL_SCANCODE_A; sc <= SDL_SCANCODE_Z; ++sc) {
                if (edge.pressed(st, static_cast<SDL_Scancode>(sc))) {
                    core::feedNameChar(app, static_cast<char>('a' + sc - SDL_SCANCODE_A));
                }
            }
            for (int sc = SDL_SCANCODE_1; sc <= SDL_SCANCODE_0; ++sc) {
                if (edge.pressed(st, static_cast<SDL_Scancode>(sc))) {
                    const int d = sc == SDL_SCANCODE_0 ? 0 : 1 + sc - SDL_SCANCODE_1;
                    core::feedNameChar(app, static_cast<char>('0' + d));
                }
            }
            if (edge.pressed(st, SDL_SCANCODE_BACKSPACE)) core::nameBackspace(app);
        }

        struct Map { SDL_Scancode sc; core::AppKey k; };
        static const Map maps[] = {
            {SDL_SCANCODE_UP, core::AppKey::Up},
            {SDL_SCANCODE_DOWN, core::AppKey::Down},
            {SDL_SCANCODE_LEFT, core::AppKey::Left},
            {SDL_SCANCODE_RIGHT, core::AppKey::Right},
            {SDL_SCANCODE_SPACE, core::AppKey::Confirm},
            {SDL_SCANCODE_RETURN, core::AppKey::Confirm},
            {SDL_SCANCODE_ESCAPE, core::AppKey::Back},
        };
        for (const auto& m : maps) {
            if (edge.pressed(st, m.sc)) core::handleKey(app, m.k, now, core::xorShift32);
        }
        if (!naming) {  // H, S et L sont des lettres pendant la saisie
            if (edge.pressed(st, SDL_SCANCODE_H))
                core::handleKey(app, core::AppKey::Help, now, core::xorShift32);
            if (edge.pressed(st, SDL_SCANCODE_S))
                core::handleKey(app, core::AppKey::Settings, now, core::xorShift32);
            if (edge.pressed(st, SDL_SCANCODE_L))
                core::handleKey(app, core::AppKey::Board, now, core::xorShift32);
        } else {
            edge.pressed(st, SDL_SCANCODE_H);  // consomme les fronts pour ne
            edge.pressed(st, SDL_SCANCODE_S);  // pas déclencher en sortant
            edge.pressed(st, SDL_SCANCODE_L);
        }

        if (app.quitRequested) {
            saveApp(app);
            requestQuit();
            return 0;
        }

        core::tickApp(app, now, core::xorShift32);
        // Pas de haut-parleur au simulateur : les files sont vidées pour ne
        // pas déborder, le son se juge sur l'appareil.
        while (core::takeCue(app.game) != core::Cue::None) {}
        while (core::takeVideoCue(app.video) != core::Cue::None) {}
        while (core::takeBjCue(app.bj) != core::Cue::None) {}
        if (app.dirty && app.game.phase != core::Phase::Spinning &&
            app.video.phase != core::Phase::Spinning) saveApp(app);

        ui::drawApp(canvas, app, now);
        canvas.pushSprite(0, 0);
        SDL_Delay(core::kFrameMs);
    }
    return 0;
}

}  // namespace

int main(int argc, char** argv) {
    for (int i = 1; i < argc; ++i) {
        if (!std::strcmp(argv[i], "--shot") && i + 1 < argc) {
            g_outDir = argv[++i];
            g_mode = Mode::Shot;
            g_frameCount = 1;
        } else if (!std::strcmp(argv[i], "--frames") && i + 2 < argc) {
            g_outDir = argv[++i];
            g_frameCount = std::atoi(argv[++i]);
            g_mode = Mode::Frames;
        } else if (!std::strcmp(argv[i], "--screens") && i + 1 < argc) {
            g_outDir = argv[++i];
            g_mode = Mode::Screens;
        } else {
            std::fprintf(stderr,
                         "usage: %s [--shot <dir>] [--frames <dir> <n>] [--screens <dir>]\n",
                         argv[0]);
            return 2;
        }
    }
    if (g_mode != Mode::Interactive) return runCapture();
    return lgfx::Panel_sdl::main(simRun);
}

#endif  // SIM_BUILD
