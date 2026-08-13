#include "sound.h"

namespace core {

namespace {

// Départ : montée courte, comme un mécanisme qu'on lance.
constexpr Tone kSpinStart[] = {{900, 40}, {1150, 40}, {1400, 60}};

// Arrêts : un « clac » sec, plus aigu à chaque rouleau. Trois notes
// distinctes valent mieux qu'une répétée — l'oreille compte la cascade.
constexpr Tone kStop1[] = {{1300, 45}};
constexpr Tone kStop2[] = {{1550, 45}};
constexpr Tone kStop3[] = {{1850, 60}};

// Gains : l'escalade sonore double l'escalade visuelle (D-008).
constexpr Tone kWinSmall[] = {{1600, 60}, {2000, 90}};
constexpr Tone kWinMid[] = {{1600, 60}, {2000, 60}, {2400, 120}};
constexpr Tone kWinBig[] = {{1200, 55}, {1600, 55}, {2000, 55}, {2400, 160}};
constexpr Tone kJackpot[] = {
    {1600, 70}, {2000, 70}, {2400, 70}, {2000, 70},
    {2400, 70}, {2600, 90}, {2400, 70}, {2600, 180},
};

// Renflouement : trois notes qui remontent — la machine redémarre, elle
// ne sanctionne pas.
constexpr Tone kBailout[] = {{1000, 90}, {1300, 90}, {1700, 140}};

constexpr Tone kBetChange[] = {{2000, 30}};

// Le cliquetis de la bille : le plus court son du jeu.
constexpr Tone kTick[] = {{2400, 14}};

// Le rachat : des pièces qui dégringolent — aigu, irrégulier, joyeux.
// Tout en haut du registre, là où le petit HP tinte le mieux.
constexpr Tone kTopup[] = {
    {2600, 30}, {2200, 25}, {2500, 30}, {2300, 25},
    {2600, 35}, {2400, 30}, {2600, 90},
};

}  // namespace

Cadence cadenceOf(Cue c) {
    switch (c) {
        case Cue::SpinStart: return {kSpinStart, 3};
        case Cue::ReelStop1: return {kStop1, 1};
        case Cue::ReelStop2: return {kStop2, 1};
        case Cue::ReelStop3: return {kStop3, 1};
        case Cue::WinSmall: return {kWinSmall, 2};
        case Cue::WinMid: return {kWinMid, 3};
        case Cue::WinBig: return {kWinBig, 4};
        case Cue::Jackpot: return {kJackpot, 8};
        case Cue::Bailout: return {kBailout, 3};
        case Cue::BetChange: return {kBetChange, 1};
        case Cue::Tick: return {kTick, 1};
        case Cue::Topup: return {kTopup, 7};
        case Cue::None: break;
    }
    return {nullptr, 0};
}

uint16_t cadenceMs(Cue c) {
    const Cadence cd = cadenceOf(c);
    uint16_t total = 0;
    for (uint8_t i = 0; i < cd.count; ++i) total = static_cast<uint16_t>(total + cd.tones[i].ms);
    return total;
}

Cue reelStopCue(uint8_t reelIndex) {
    switch (reelIndex) {
        case 0: return Cue::ReelStop1;
        case 1: return Cue::ReelStop2;
        default: return Cue::ReelStop3;
    }
}

}  // namespace core
