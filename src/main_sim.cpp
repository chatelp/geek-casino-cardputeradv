// Main simulateur macOS — LovyanGFX + SDL2 (Panel_sdl), fenêtre ×3.
//
//   (aucune option)      fenêtre interactive : espace = tirer, ←/→ = mise
//   --shot <dir>         une image déterministe, sans fenêtre
//   --frames <dir> <n>   n images déterministes, pour un GIF
//
// Les captures n'ouvrent aucune fenêtre et avancent le temps d'un pas fixe :
// même commande, mêmes pixels, toujours.
#ifdef SIM_BUILD

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

#include <lgfx/v1/platforms/sdl/Panel_sdl.hpp>

#include "game.h"
#include "hal_display.h"
#include "hal_keys.h"
#include "rng.h"
#include "slot_screen.h"

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

// Panel_sdl::main ne rend la main que fenêtres fermées : on pousse un
// SDL_QUIT pour terminer proprement depuis le thread utilisateur.
void requestQuit() {
    SDL_Event ev;
    ev.type = SDL_QUIT;
    SDL_PushEvent(&ev);
}

std::string g_outDir;
int g_frameCount = 0;  // 0 = interactif, sinon nombre d'images à écrire
constexpr uint32_t kFixedSeed = 0xCA51704Du;

// Écrit un BMP 24 bits. readRect renvoie du RGB565 aux octets inversés
// (piège documenté dans CLAUDE.md) : on corrige ici.
bool writeBmp(lgfx::LGFX_Sprite& g, const std::string& path) {
    const int w = g.width(), h = g.height();
    static uint16_t px[hal::kScreenW * hal::kScreenH];
    g.readRect(0, 0, w, h, px);

    const uint32_t rowBytes = ((w * 3 + 3) / 4) * 4;
    const uint32_t imageSize = rowBytes * h;
    uint8_t hdr[54] = {'B', 'M'};
    auto put32 = [&](int off, uint32_t v) {
        hdr[off] = v; hdr[off + 1] = v >> 8; hdr[off + 2] = v >> 16; hdr[off + 3] = v >> 24;
    };
    put32(2, 54 + imageSize); put32(10, 54); put32(14, 40);
    put32(18, static_cast<uint32_t>(w)); put32(22, static_cast<uint32_t>(h));
    hdr[26] = 1; hdr[28] = 24;
    put32(34, imageSize);

    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) return false;
    std::fwrite(hdr, 1, 54, f);
    static uint8_t row[((hal::kScreenW * 3 + 3) / 4) * 4];
    for (int y = h - 1; y >= 0; --y) {  // BMP : lignes de bas en haut
        std::memset(row, 0, sizeof(row));
        for (int x = 0; x < w; ++x) {
            const uint16_t s = px[y * w + x];
            const uint16_t c = static_cast<uint16_t>((s >> 8) | (s << 8));
            row[x * 3 + 0] = static_cast<uint8_t>(((c >> 0) & 0x1F) * 255 / 31);   // B
            row[x * 3 + 1] = static_cast<uint8_t>(((c >> 5) & 0x3F) * 255 / 63);   // G
            row[x * 3 + 2] = static_cast<uint8_t>(((c >> 11) & 0x1F) * 255 / 31);  // R
        }
        std::fwrite(row, 1, rowBytes, f);
    }
    std::fclose(f);
    return true;
}

// Capture sans fenêtre : temps simulé par pas fixes, graine fixe. Le tour
// est lancé à la première image pour que la suite montre l'animation.
int runCapture() {
    core::seedXorShift(kFixedSeed);
    lgfx::LGFX_Sprite canvas(nullptr);
    canvas.setColorDepth(16);
    canvas.createSprite(hal::kScreenW, hal::kScreenH);

    uint32_t now = 0;
    core::Game game = core::newGame(now, core::xorShift32);
    core::startSpin(game, now, core::xorShift32);

    for (int i = 0; i < g_frameCount; ++i) {
        core::updateGame(game, now, core::xorShift32);
        // Enchaîner les tours : une capture doit montrer la machine en
        // marche, pas l'attendre pendant le délai du mode démo.
        if (game.phase == core::Phase::Idle) {
            core::startSpin(game, now, core::xorShift32);
        }
        ui::drawSlotScreen(canvas, game, now);
        char name[32];
        if (g_frameCount == 1) std::snprintf(name, sizeof(name), "/shot.bmp");
        else std::snprintf(name, sizeof(name), "/frame_%04d.bmp", i);
        if (!writeBmp(canvas, g_outDir + name)) {
            std::fprintf(stderr, "capture : echec d'ecriture dans %s\n", g_outDir.c_str());
            return 1;
        }
        now += core::kFrameMs;
    }
    return 0;
}

hal::Key readKey() {
    const uint8_t* k = SDL_GetKeyboardState(nullptr);
    if (k[SDL_SCANCODE_ESCAPE] || k[SDL_SCANCODE_Q]) return hal::Key::Escape;
    if (k[SDL_SCANCODE_SPACE]) return hal::Key::Space;
    if (k[SDL_SCANCODE_RETURN]) return hal::Key::Enter;
    if (k[SDL_SCANCODE_LEFT]) return hal::Key::Left;
    if (k[SDL_SCANCODE_RIGHT]) return hal::Key::Right;
    return hal::Key::None;
}

int simRun(bool* running) {
    core::seedXorShift(kFixedSeed);

    SimDisplay display;
    display.init();
    lgfx::LGFX_Sprite canvas(&display);
    canvas.setColorDepth(16);
    canvas.createSprite(hal::kScreenW, hal::kScreenH);

    const uint32_t t0 = SDL_GetTicks();
    core::Game game = core::newGame(0, core::xorShift32);
    hal::Key prev = hal::Key::None;

    while (*running) {
        const uint32_t now = SDL_GetTicks() - t0;
        const hal::Key key = readKey();
        if (key != prev) {  // front montant : une pression = une action
            switch (key) {
                case hal::Key::Escape:
                    requestQuit();
                    return 0;
                case hal::Key::Space:
                case hal::Key::Enter:
                    core::noteInput(game, now);
                    core::startSpin(game, now, core::xorShift32);
                    break;
                case hal::Key::Left:
                    core::noteInput(game, now);
                    core::lowerBet(game.machine.econ);
                    break;
                case hal::Key::Right:
                    core::noteInput(game, now);
                    core::raiseBet(game.machine.econ);
                    break;
                default:
                    break;
            }
            prev = key;
        }
        core::updateGame(game, now, core::xorShift32);
        ui::drawSlotScreen(canvas, game, now);
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
            g_frameCount = 1;
        } else if (!std::strcmp(argv[i], "--frames") && i + 2 < argc) {
            g_outDir = argv[++i];
            g_frameCount = std::atoi(argv[++i]);
        } else {
            std::fprintf(stderr, "usage: %s [--shot <dir>] [--frames <dir> <n>]\n", argv[0]);
            return 2;
        }
    }
    if (g_frameCount > 0) return runCapture();
    return lgfx::Panel_sdl::main(simRun);
}

#endif  // SIM_BUILD
