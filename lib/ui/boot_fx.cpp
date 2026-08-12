#include "boot_fx.h"

#include "layout.h"
#include "painter.h"
#include "palette.h"
#include "symbol_ids.h"
#include "symbols.h"

namespace ui {

using layout::kScreenH;
using layout::kScreenW;
namespace P = pal;

namespace {

// Palette de déchets : les couleurs saturées d'une mémoire vidéo non
// initialisée, prises dans notre propre palette pour que même le chaos
// reste de la maison.
constexpr uint16_t kJunk[8] = {
    P::magenta, P::cyan, P::yellow, P::green,
    P::orange, P::violet, P::red, P::white,
};

// Blocs de bruit, en 4x4 : à 240x135 un bruit au pixel serait illisible
// et coûterait 32 000 rectangles par image.
void drawNoise(lgfx::LGFX_Sprite& g, uint32_t frame, float density) {
    constexpr int kB = 4;
    for (int y = 0; y < kScreenH; y += kB) {
        for (int x = 0; x < kScreenW; x += kB) {
            const uint32_t h = core::bootHash(x, y, frame);
            if ((h & 0xFFu) > static_cast<uint32_t>(density * 255.0f)) continue;
            g.fillRect(x, y, kB, kB, kJunk[(h >> 8) & 7]);
        }
    }
}

// Barres de couleur, avec des lignes déchirées : le balayage n'est pas
// encore verrouillé.
void drawBars(lgfx::LGFX_Sprite& g, uint32_t frame, float p) {
    const int bw = kScreenW / 8;
    for (int i = 0; i < 8; ++i) {
        g.fillRect(i * bw, 0, bw, kScreenH, kJunk[i]);
    }
    // Déchirures : quelques bandes horizontales décalées.
    for (int i = 0; i < 6; ++i) {
        const uint32_t h = core::bootHash(0, i, frame);
        const int y = static_cast<int>(h % kScreenH);
        const int th = 3 + static_cast<int>((h >> 8) % 9);
        const int off = static_cast<int>((h >> 16) % 41) - 20;
        for (int yy = y; yy < y + th && yy < kScreenH; ++yy) {
            for (int x = 0; x < kScreenW; x += bw) {
                const int src = ((x + off) / bw + 8) % 8;
                g.fillRect(x, yy, bw, 1, kJunk[src]);
            }
        }
    }
    // Les barres s'effacent par le bas au fil de la phase.
    const int wipe = static_cast<int>(kScreenH * p);
    if (wipe > 0) g.fillRect(0, kScreenH - wipe, kScreenW, wipe, P::ink900);
}

// Faux test mémoire. Les chiffres défilent puis se figent sur OK — c'est
// le passage que tout le monde reconnaît.
void drawSelftest(lgfx::LGFX_Sprite& g, uint32_t frame, float p) {
    g.fillScreen(P::ink900);
    // Le test est faux, les CHIFFRES sont vrais : ils sont tous vérifiés
    // contre le matériel réel (définition de carte m5stack-stamps3, pilotes
    // instanciés par M5GFX et M5Cardputer). Un faux écran de démarrage qui
    // annonce du matériel inexistant, c'est du décor ; un qui annonce le
    // vrai, c'est une carte d'identité — et c'est bien plus geek.
    //
    // « ST7789V2 » figurait ici sans preuve : M5GFX instancie un
    // Panel_ST7789 et ne connaît pas de révision. On n'affiche que ce qu'on
    // peut montrer.
    static const char* kLines[] = {
        "GEEK CASINO BOOT ROM V1.0",
        "CPU   ESP32-S3  240 MHZ",
        "FLASH 8 MB   PSRAM NONE",
        "VRAM  64800 BYTES",
        "LCD   ST7789  240X135",
        "KBD   TCA8418 I2C 0X34",
        "IMU   BMI270",
        "SND   1W  800-2600 HZ",
    };
    constexpr int kn = sizeof(kLines) / sizeof(kLines[0]);
    const int shown = 1 + static_cast<int>(p * kn * 1.4f);
    for (int i = 0; i < kn && i < shown; ++i) {
        const int y = 14 + i * 13;
        drawText(g, kLines[i], 12, y, P::greenDk, 1);
        if (i + 1 < shown) {
            drawText(g, "OK", kScreenW - 14, y, P::green, 1, Align::Right);
        } else {
            // La ligne en cours affiche un compteur qui grimpe.
            const uint32_t v = core::bootHash(i, 0, frame) % 65536u;
            drawNumber(g, static_cast<int32_t>(v), kScreenW - 14, y, P::greenDk, 1,
                       Align::Right);
        }
    }
    // Curseur clignotant, comme une console qui attend.
    if ((frame / 8) & 1) g.fillRect(12, 14 + shown * 13, 5, 7, P::green);
}

// Le logo émerge : le bruit se retire, les lettres restent.
void drawLogo(lgfx::LGFX_Sprite& g, uint32_t frame, float p) {
    g.fillScreen(P::ink900);
    // Bruit résiduel : il retombe au carré, sinon le nom reste noyé
    // pendant la moitié de la phase.
    const float fade = (1.0f - p) * (1.0f - p);
    drawNoise(g, frame, fade * 0.30f);

    const int cy = kScreenH / 2 - 16;
    drawText(g, "GEEK", kScreenW / 2, cy, P::magenta, 3, Align::Center);
    drawText(g, "CASINO", kScreenW / 2, cy + 24, P::cyan, 3, Align::Center);

    // Trois invaders sous le nom, qui apparaissent l'un après l'autre.
    const int n = static_cast<int>(p * 4.0f);
    for (int i = 0; i < 3 && i < n; ++i) {
        drawSymbol(g, core::SYM_INVADER, kScreenW / 2 - 40 + i * 28, cy + 48, 1);
    }
}

}  // namespace

void drawBootFx(lgfx::LGFX_Sprite& g, uint32_t elapsed) {
    // L'image sert de graine : à la même milliseconde, la même image.
    const uint32_t frame = elapsed / 33u;
    const float p = core::bootPhaseProgress(elapsed);

    switch (core::bootPhase(elapsed)) {
        case core::BootPhase::Noise:
            g.fillScreen(P::ink900);
            // La densité monte puis retombe : l'écran « se charge ».
            drawNoise(g, frame, p < 0.5f ? p * 1.6f : (1.0f - p) * 1.6f);
            break;
        case core::BootPhase::Bars:
            drawBars(g, frame, p);
            break;
        case core::BootPhase::Selftest:
            drawSelftest(g, frame, p);
            break;
        case core::BootPhase::Logo:
            drawLogo(g, frame, p);
            break;
        case core::BootPhase::Done:
            g.fillScreen(P::ink900);
            break;
    }
}

}  // namespace ui
