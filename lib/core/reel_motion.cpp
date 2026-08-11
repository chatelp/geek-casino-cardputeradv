#include "reel_motion.h"

#include <cmath>

namespace core {

namespace {

// Décélération cubique : rapide au début, freinage long à la fin.
// Vaut exactement 1 en p = 1 — c'est ce qui garantit l'arrêt pile sur un
// symbole, sans dérive accumulée.
float ease(float p) {
    if (p <= 0.0f) return 0.0f;
    if (p >= 1.0f) return 1.0f;
    const float q = 1.0f - p;
    return 1.0f - q * q * q;
}

// Cloche de dépassement : nulle aux deux bouts, maximale au milieu du
// dernier segment. Nulle en p = 1, donc elle ne déplace pas l'arrêt.
float overshoot(float p) {
    if (p <= kOvershootStart || p >= 1.0f) return 0.0f;
    const float k = (p - kOvershootStart) / (1.0f - kOvershootStart);
    return std::sin(k * 3.14159265f) * (1.0f - k);
}

}  // namespace

ReelMotion armReel(uint32_t now, uint8_t reel, uint16_t from, uint16_t to,
                   uint16_t stripLen, uint8_t minTurns) {
    ReelMotion m;
    m.startPos = static_cast<float>(from);
    m.t0 = now;
    m.dur = kSpinBaseMs + static_cast<uint32_t>(reel) * kSpinStaggerMs;

    // Distance entière : tours complets, puis le reste jusqu'à la cible.
    int32_t delta = static_cast<int32_t>(to) - static_cast<int32_t>(from);
    while (delta < 0) delta += stripLen;
    m.travel = static_cast<float>(static_cast<int32_t>(minTurns) * stripLen + delta);
    if (m.dur == 0) m.dur = 1;
    return m;
}

float reelPosition(const ReelMotion& m, uint32_t now) {
    if (now <= m.t0) return m.startPos;
    const uint32_t dt = now - m.t0;
    if (dt >= m.dur) return m.startPos + m.travel;  // exact, pas d'approche
    const float p = static_cast<float>(dt) / static_cast<float>(m.dur);
    return m.startPos + m.travel * ease(p) + kOvershootSymbols * overshoot(p);
}

bool reelSettled(const ReelMotion& m, uint32_t now) {
    return now >= m.t0 + m.dur;
}

}  // namespace core
