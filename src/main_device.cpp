// Main appareil — seul fichier autorisé à toucher M5Cardputer.
#ifdef ARDUINO

#include <M5Cardputer.h>

#include "boot_screen.h"
#include "hal_display.h"

// Constructeur global : ne stocke qu'un pointeur, aucun appel d'API M5
// (M5 n'est pas encore initialisé — voir CLAUDE.md).
static M5Canvas canvas(&M5Cardputer.Display);
static uint32_t frame = 0;

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5Cardputer.Display.setRotation(1);
    canvas.setColorDepth(16);
    canvas.createSprite(hal::kScreenW, hal::kScreenH);
}

void loop() {
    M5Cardputer.update();
    ui::drawBootScreen(canvas, frame++);
    canvas.pushSprite(0, 0);
    delay(16);
}

#endif  // ARDUINO
