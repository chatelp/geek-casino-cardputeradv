#include "scope.h"

namespace core {

namespace {

// Onde triangulaire, -100..100. Une trace doit ressembler à un SIGNAL :
// du bruit pur se lit comme un défaut d'affichage, pas comme une machine
// qui travaille.
int32_t tri(uint32_t i, uint32_t period) {
    const uint32_t k = i % period;
    const uint32_t half = period / 2;
    const int32_t up = static_cast<int32_t>((k * 200) / half) - 100;
    const int32_t down = 100 - static_cast<int32_t>(((k - half) * 200) / half);
    return k < half ? up : down;
}

// Le repos ne dépasse jamais ce tiers de la demi-hauteur. C'est CE
// plafond qui donne son amplitude aux salves : sans lui, elles n'auraient
// nulle part où aller — la faute exacte de l'analyseur de spectre qui
// précédait.
constexpr int32_t kRestCeiling = 34;

}  // namespace

ScopeDrive scopeDriveOfReels(const ReelMotion* m, uint8_t reels, uint32_t now) {
    ScopeDrive d;
    if (reels == 0) return d;
    uint8_t flying = 0;
    for (uint8_t i = 0; i < reels && d.bursts < kScopeBursts; ++i) {
        const uint32_t stopAt = m[i].t0 + m[i].dur;
        if (now < stopAt) {
            ++flying;
        } else {
            d.burstMs[d.bursts++] = stopAt;
        }
    }
    // Un rouleau encore en vol suffit à agiter la trace ; tous en vol la
    // secouent à plein. Quand il n'en reste plus, seules les salves
    // achèvent de défiler.
    d.energy = flying == 0
                   ? 0
                   : static_cast<uint8_t>(40 + (60 * flying) / reels);
    return d;
}

ScopeDrive scopeDriveOfWin(uint8_t tier, uint32_t now) {
    ScopeDrive d;
    if (tier == 0) return d;
    d.energy = 55;
    const uint8_t want = tier >= 4 ? 5 : tier;
    // Instants alignés sur une grille de 300 ms : le train de salves défile
    // sans dépendre de l'image à laquelle on interroge la fonction, donc
    // deux captures du même instant donnent la même trace.
    const uint32_t base = (now / 300u) * 300u;
    for (uint8_t i = 0; i < want && i < kScopeBursts; ++i) {
        const uint32_t t = i * 300u;
        if (t > base) break;
        d.burstMs[d.bursts++] = base - t;
    }
    return d;
}

int8_t scopeAt(int x, int w, uint32_t now, const ScopeDrive& d) {
    // Le fond défile : l'indice avance avec le temps, pas la colonne.
    const uint32_t idx = static_cast<uint32_t>(x) + now / kScopeMsPerPx;
    // Deux périodes premières entre elles : le motif ne se répète pas à
    // l'œil, sans qu'il faille pour autant du hasard.
    int32_t v = (tri(idx, 11) * 62 + tri(idx, 29) * 38) / 100;
    v = (v * d.energy) / 100;
    v = (v * kRestCeiling) / 100;

    // Les salves écrasent le repos là où elles passent — une explosion ne
    // se mélange pas au fond, elle le remplace.
    for (uint8_t i = 0; i < d.bursts; ++i) {
        if (now < d.burstMs[i]) continue;
        const uint32_t age = now - d.burstMs[i];
        const int32_t xs = (w - 1) - static_cast<int32_t>(age / kScopeMsPerPx);
        int32_t dist = x - xs;
        if (dist < 0) dist = -dist;
        if (dist >= kScopeBurstW) continue;
        // Décroissance au carré : un pic franc, pas une bosse molle.
        const int32_t k = kScopeBurstW - dist;
        const int32_t env = (k * k * 100) / (kScopeBurstW * kScopeBurstW);
        // Une colonne sur deux de part et d'autre : c'est cette oscillation
        // dense qui fait lire « salve » et non « pointe ».
        const int32_t osc = (x & 1) ? env : -env;
        if ((osc < 0 ? -osc : osc) > (v < 0 ? -v : v)) v = osc;
    }
    if (v > 100) v = 100;
    if (v < -100) v = -100;
    return static_cast<int8_t>(v);
}

}  // namespace core
