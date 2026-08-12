#include "boot.h"

namespace core {

BootPhase bootPhase(uint32_t t) {
    if (t < kBootNoiseMs) return BootPhase::Noise;
    if (t < kBootNoiseMs + kBootBarsMs) return BootPhase::Bars;
    if (t < kBootNoiseMs + kBootBarsMs + kBootTestMs) return BootPhase::Selftest;
    if (t < kBootTotalMs) return BootPhase::Logo;
    return BootPhase::Done;
}

float bootPhaseProgress(uint32_t t) {
    uint32_t start = 0, dur = kBootNoiseMs;
    switch (bootPhase(t)) {
        case BootPhase::Noise: break;
        case BootPhase::Bars:
            start = kBootNoiseMs; dur = kBootBarsMs; break;
        case BootPhase::Selftest:
            start = kBootNoiseMs + kBootBarsMs; dur = kBootTestMs; break;
        case BootPhase::Logo:
            start = kBootNoiseMs + kBootBarsMs + kBootTestMs; dur = kBootLogoMs; break;
        case BootPhase::Done: return 1.0f;
    }
    if (t <= start) return 0.0f;
    const uint32_t d = t - start;
    return d >= dur ? 1.0f : static_cast<float>(d) / static_cast<float>(dur);
}

uint32_t bootHash(int32_t x, int32_t y, uint32_t frame) {
    // Mélange entier bon marché — pas besoin de qualité statistique, il
    // faut seulement que ce soit reproductible et sans motif visible.
    uint32_t h = static_cast<uint32_t>(x) * 73856093u;
    h ^= static_cast<uint32_t>(y) * 19349663u;
    h ^= frame * 83492791u;
    h ^= h >> 13;
    h *= 1274126177u;
    return h ^ (h >> 16);
}

}  // namespace core
