// Composition sonore — logique pure. La synthèse PCM vit dans lib/hal.
//
// CONTRAINTE MATÉRIELLE MESURÉE : le haut-parleur de 1 W ne restitue
// pratiquement rien sous ~400 Hz. Toute la composition tient entre 800 et
// 2600 Hz. Ce n'est pas une préférence esthétique, c'est le seul registre
// audible — et un test le vérifie pour chaque note.
#pragma once
#include <cstdint>

namespace core {

constexpr uint16_t kAudibleMinHz = 800;
constexpr uint16_t kAudibleMaxHz = 2600;
constexpr uint16_t kSampleRate = 22050;
// Plafond de durée d'un motif : dimensionne le tampon PCM statique.
constexpr uint16_t kMaxCueMs = 700;

enum class Cue : uint8_t {
    None = 0,
    SpinStart,
    ReelStop1,
    ReelStop2,
    ReelStop3,
    WinSmall,
    WinMid,
    WinBig,
    Jackpot,
    Bailout,
    BetChange,
    // Passage d'une case sous la bille. Très court : il en part
    // plusieurs par seconde au début du lancer.
    Tick,
};

struct Tone {
    uint16_t hz;
    uint16_t ms;
};

struct Cadence {
    const Tone* tones;
    uint8_t count;
};

Cadence cadenceOf(Cue c);
uint16_t cadenceMs(Cue c);

// Le son d'arrêt monte d'un rouleau à l'autre : la cascade s'entend même
// les yeux fermés.
Cue reelStopCue(uint8_t reelIndex);

}  // namespace core
