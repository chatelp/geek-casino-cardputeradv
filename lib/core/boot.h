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

// Les glitchs sont du décor, le test est de la LECTURE : le budget penche
// donc du côté du test. Huit lignes en 750 ms faisaient moins de 100 ms
// chacune — de quoi voir défiler, pas de quoi lire. Le total ne bouge que
// de 500 ms parce que les deux phases de bruit ont rendu du temps.
constexpr uint32_t kBootNoiseMs = 380;
constexpr uint32_t kBootBarsMs = 440;
constexpr uint32_t kBootTestMs = 1560;
constexpr uint32_t kBootLogoMs = 780;
constexpr uint32_t kBootTotalMs =
    kBootNoiseMs + kBootBarsMs + kBootTestMs + kBootLogoMs;

BootPhase bootPhase(uint32_t elapsedMs);
// Avancement DANS la phase courante, 0 à 1.
float bootPhaseProgress(uint32_t elapsedMs);

// Bruit déterministe : même image à la même milliseconde. Sans ça, ni
// capture reproductible ni test possible.
uint32_t bootHash(int32_t x, int32_t y, uint32_t frame);

}  // namespace core
