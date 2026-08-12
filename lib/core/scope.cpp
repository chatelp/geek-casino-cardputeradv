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

// Hachage entier, même esprit que celui de l'écran d'allumage : du bruit
// qui ne dépend que de ses entrées. Un générateur à état donnerait des
// captures différentes à chaque lancement.
uint32_t noise(uint32_t a, uint32_t b) {
    uint32_t h = a * 2654435761u ^ b * 2246822519u;
    h ^= h >> 13;
    h *= 3266489917u;
    h ^= h >> 16;
    return h;
}

// Écart signé -100..100 tiré du hachage.
int32_t swing(uint32_t a, uint32_t b) {
    return static_cast<int32_t>(noise(a, b) % 201u) - 100;
}

// Le repos ne dépasse jamais ce tiers de la demi-hauteur. C'est CE
// plafond qui donne son amplitude aux secousses : sans lui, elles
// n'auraient nulle part où aller — la faute exacte de l'analyseur de
// spectre qui précédait.
constexpr int32_t kRestCeiling = 34;

}  // namespace

ScopeDrive scopeDriveOfReels(const ReelMotion* m, uint8_t reels, uint32_t now) {
    ScopeDrive d;
    if (reels == 0) return d;
    uint8_t flying = 0;
    for (uint8_t i = 0; i < reels && d.shocks < kScopeShocks; ++i) {
        const uint32_t stopAt = m[i].t0 + m[i].dur;
        if (now < stopAt) ++flying;
        else d.shockMs[d.shocks++] = stopAt;
    }
    // Un rouleau encore en vol suffit à agiter la trace ; tous en vol la
    // secouent à plein. Quand il n'en reste plus, la trace se recale.
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
    // Instants alignés sur une grille de 300 ms : le train de secousses se
    // rejoue sans dépendre de l'image à laquelle on interroge la fonction,
    // donc deux captures du même instant donnent la même trace.
    const uint32_t base = (now / 300u) * 300u;
    for (uint8_t i = 0; i < want && i < kScopeShocks; ++i) {
        const uint32_t back = i * 300u;
        if (back > base) break;
        d.shockMs[d.shocks++] = base - back;
    }
    return d;
}

uint8_t scopeShock(uint32_t now, const ScopeDrive& d) {
    uint32_t best = 0;
    for (uint8_t i = 0; i < d.shocks; ++i) {
        if (now < d.shockMs[i]) continue;
        const uint32_t age = now - d.shockMs[i];
        if (age >= kScopeShockMs) continue;
        // Décroissance au carré : la secousse frappe fort puis lâche vite.
        // Linéaire, elle traînait et ressemblait à un état.
        const uint32_t k = kScopeShockMs - age;
        const uint32_t s = (k * k * 100u) / (kScopeShockMs * kScopeShockMs);
        if (s > best) best = s;
    }
    return static_cast<uint8_t>(best);
}

int8_t scopeAt(int x, int w, uint32_t now, const ScopeDrive& d) {
    (void)w;
    // Le signal de repos défile : l'indice avance avec le temps, pas la
    // colonne. Deux périodes premières entre elles pour que le motif ne se
    // répète pas à l'œil, sans qu'il faille du hasard.
    const uint32_t idx = static_cast<uint32_t>(x) + now / kScopeMsPerPx;
    int32_t v = (tri(idx, 11) * 62 + tri(idx, 29) * 38) / 100;
    v = (v * d.energy) / 100;
    v = (v * kRestCeiling) / 100;

    const int32_t shock = scopeShock(now, d);
    if (shock > 0) {
        const uint32_t tick = now / kScopeShockStepMs;
        // DÉCHIRURE : des blocs entiers sautent, comme un balayage qui perd
        // sa synchro. C'est elle qui fait lire « brouillage » — un bruit
        // colonne par colonne seul ne donnerait qu'un flou uniforme.
        //
        // L'écart est tiré LOIN DE ZÉRO (55 à 100, d'un côté ou de
        // l'autre). Un tirage uniforme se serait massé autour du milieu :
        // la première version brouillait à peine, parce que la plupart des
        // blocs ne bougeaient presque pas.
        const uint32_t h = noise(static_cast<uint32_t>(x) / kScopeTearW, tick);
        const int32_t mag = 55 + static_cast<int32_t>(h % 46u);
        const int32_t tear = (h & 0x10000u) ? mag : -mag;
        // GRÉSILLEMENT : par-dessus, une vibration fine à l'intérieur du
        // bloc, pour que la déchirure ne soit pas un simple créneau.
        const int32_t grain =
            (swing(static_cast<uint32_t>(x) + 911u, tick + 7u) * 22) / 100;
        // Pendant la secousse, le brouillage EST la trace : il remplace le
        // signal de repos au lieu de s'y ajouter.
        v = ((tear + grain) * shock) / 100;
    }

    if (v > 100) v = 100;
    if (v < -100) v = -100;
    return static_cast<int8_t>(v);
}

}  // namespace core
