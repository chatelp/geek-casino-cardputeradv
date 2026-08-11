// GÉNÉRÉ par design/tools/gen.py — ne pas éditer à la main.
// Source de vérité : design/tokens.json, design/tools/art_*.py
#pragma once
#include <cstdint>

namespace core {

// Ordre = ordre de valeur croissante. INVADER est le jackpot.
enum Symbol : uint8_t {
    SYM_RESISTOR  = 0,  // Résistance — Électronique / maker
    SYM_LED       = 1,  // LED — Électronique / maker
    SYM_CHIP      = 2,  // Puce — Électronique / maker
    SYM_FLOPPY    = 3,  // Disquette — Rétro-computing
    SYM_GAMEPAD   = 4,  // Manette — Rétro-gaming
    SYM_CRT       = 5,  // Écran CRT — Rétro-computing
    SYM_D20       = 6,  // Dé 20 — Geek pop
    SYM_INVADER   = 7,  // Invader — Geek pop — JACKPOT
};

constexpr uint8_t kSymbolCount = 8;
constexpr Symbol kJackpotSymbol = SYM_INVADER;

}  // namespace core
