// Main appareil — seul fichier autorisé à toucher M5Cardputer.
// Il ne contient aucune règle de jeu : il branche l'horloge, l'aléa et le
// clavier sur le même core::Game que le simulateur.
#ifdef ARDUINO

#include <M5Cardputer.h>
#include <esp_random.h>

#include "game.h"
#include "hal_display.h"
#include "hal_keys.h"
#include "rng.h"
#include "slot_screen.h"

// Constructeur global : ne stocke qu'un pointeur, aucun appel d'API M5
// (M5 n'est pas encore initialisé — voir CLAUDE.md).
static M5Canvas canvas(&M5Cardputer.Display);
static core::Game game;
static hal::Key prevKey = hal::Key::None;

// TRNG matériel, injecté sous la même signature que le PRNG de test.
static uint32_t trng() { return esp_random(); }

static hal::Key readKey() {
    if (!M5Cardputer.Keyboard.isChange() && !M5Cardputer.Keyboard.isPressed()) {
        return hal::Key::None;
    }
    const auto st = M5Cardputer.Keyboard.keysState();
    if (st.enter) return hal::Key::Enter;
    for (const auto c : st.word) {
        switch (c) {
            case ' ': return hal::Key::Space;
            case ',': return hal::Key::Left;   // flèche gauche
            case '/': return hal::Key::Right;  // flèche droite
            case ';': return hal::Key::Up;
            case '.': return hal::Key::Down;
            default: break;
        }
    }
    return hal::Key::None;
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5Cardputer.Display.setRotation(1);
    // Sprite plein écran 16 bits (64,8 Ko) : tout est dessiné dedans puis
    // poussé d'un bloc. Aucun dessin direct à l'écran, aucun scintillement.
    canvas.setColorDepth(16);
    canvas.createSprite(hal::kScreenW, hal::kScreenH);
    game = core::newGame(millis(), trng);
}

void loop() {
    M5Cardputer.update();
    const uint32_t now = millis();

    const hal::Key key = readKey();
    if (key != prevKey) {  // front montant : une pression = une action
        switch (key) {
            case hal::Key::Space:
            case hal::Key::Enter:
                core::noteInput(game, now);
                core::startSpin(game, now, trng);
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
        prevKey = key;
    }

    core::updateGame(game, now, trng);
    ui::drawSlotScreen(canvas, game, now);
    canvas.pushSprite(0, 0);
    delay(core::kFrameMs);
}

#endif  // ARDUINO
