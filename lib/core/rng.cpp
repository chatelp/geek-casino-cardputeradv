#include "rng.h"

namespace core {

uint32_t drawBelow(RngFn rng, uint32_t n) {
    if (n < 2) return 0;
    // Rejette la tranche basse [0, 2^32 mod n) pour que chaque résidu
    // apparaisse le même nombre de fois.
    const uint32_t threshold = static_cast<uint32_t>(-n) % n;
    for (;;) {
        const uint32_t r = rng();
        if (r >= threshold) return r % n;
    }
}

namespace {
uint32_t g_state = 0x9E3779B9u;
}

void seedXorShift(uint32_t seed) {
    g_state = seed ? seed : 0x9E3779B9u;  // xorshift interdit l'état nul
}

uint32_t xorShift32() {
    uint32_t x = g_state;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    g_state = x;
    return x;
}

}  // namespace core
