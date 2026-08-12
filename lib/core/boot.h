// Séquence d'allumage — logique pure (le temps, les phases).
//
// Imite ce que faisaient les vieux cabinets d'arcade : quelques centaines
// de millisecondes de déchets multicolores pendant que la RAM vidéo se
// remplit, un test mémoire en phosphore, puis le logo. C'est du décor
// assumé — la machine n'a rien à tester — mais c'est exactement le genre
// de détail qui fait « vieille borne » plutôt que « application ».
#pragma once
#include <cstdint>

namespace core {

enum class BootPhase : uint8_t {
    Noise,    // bruit multicolore : la mémoire vidéo « se remplit »
    Bars,     // barres de couleur et déchirures de balayage
    Selftest, // faux test mémoire, en vert phosphore
    Logo,     // le nom émerge du bruit
    Done,
};

constexpr uint32_t kBootNoiseMs = 450;
constexpr uint32_t kBootBarsMs = 550;
constexpr uint32_t kBootTestMs = 750;
constexpr uint32_t kBootLogoMs = 900;
constexpr uint32_t kBootTotalMs =
    kBootNoiseMs + kBootBarsMs + kBootTestMs + kBootLogoMs;

BootPhase bootPhase(uint32_t elapsedMs);
// Avancement DANS la phase courante, 0 à 1.
float bootPhaseProgress(uint32_t elapsedMs);

// Bruit déterministe : même image à la même milliseconde. Sans ça, ni
// capture reproductible ni test possible.
uint32_t bootHash(int32_t x, int32_t y, uint32_t frame);

}  // namespace core
