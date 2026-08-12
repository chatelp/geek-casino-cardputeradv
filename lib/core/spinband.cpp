#include "spinband.h"

namespace core {

namespace {

// Hachage entier, même esprit que celui de l'écran d'allumage : du bruit
// qui ne dépend que de ses entrées, donc reproductible d'une exécution à
// l'autre. Un générateur à état donnerait des captures différentes à
// chaque lancement et rendrait les tests d'image impossibles.
uint32_t bandHash(uint8_t bar, uint32_t tick) {
    uint32_t h = static_cast<uint32_t>(bar) * 2654435761u ^ tick * 2246822519u;
    h ^= h >> 13;
    h *= 3266489917u;
    h ^= h >> 16;
    return h;
}

// Enveloppe en cloche : les barres du centre montent plus haut que celles
// des bords. Sans elle, le bandeau est un mur de bruit uniforme — l'œil
// n'y lit aucune forme, donc aucune intensité.
uint32_t envelope(uint8_t bar) {
    const int mid = kBandBars / 2;
    const int d = bar < mid ? mid - 1 - bar : bar - mid;
    const int far = mid - 1;
    // Creux de 38 % aux bords : assez pour dessiner une cloche, pas assez
    // pour laisser les barres extrêmes au ras du sol.
    return static_cast<uint32_t>(100 - (38 * d) / (far > 0 ? far : 1));
}

// Le coup porté quand un rouleau se verrouille : toutes les barres sautent
// au plafond puis retombent en 260 ms. C'est ce à-coup qui fait entendre
// la cascade des arrêts à l'œil.
constexpr uint32_t kSlamMs = 260;

uint32_t slamFloor(const BandDrive& d) {
    if (!d.locked || d.sinceLockMs >= kSlamMs) return 0;
    return 100u - (100u * d.sinceLockMs) / kSlamMs;
}

uint8_t levelAtTick(uint8_t bar, uint32_t tick, const BandDrive& d) {
    const uint32_t noise = bandHash(bar, tick) & 0xFF;
    // 68 % de socle, 32 % de bruit : la barre respire sans jamais
    // s'effacer. Avec trop de bruit le bandeau clignote au lieu d'onduler,
    // et surtout il paraît timide — on veut qu'il pousse.
    const uint32_t swing = 68u + (32u * noise) / 255u;
    uint32_t v = (d.energy * envelope(bar) * swing) / 10000u;
    const uint32_t floorV = slamFloor(d);
    if (v < floorV) v = floorV;
    return static_cast<uint8_t>(v > 100 ? 100 : v);
}

}  // namespace

BandDrive bandDriveOfReels(const ReelMotion* m, uint8_t reels, uint32_t now) {
    BandDrive d;
    if (reels == 0) return d;
    uint8_t flying = 0;
    uint32_t lastLock = 0;
    for (uint8_t i = 0; i < reels; ++i) {
        const uint32_t stopAt = m[i].t0 + m[i].dur;
        if (now < stopAt) {
            ++flying;
        } else if (!d.locked || stopAt > lastLock) {
            d.locked = true;
            lastLock = stopAt;
        }
    }
    if (d.locked) d.sinceLockMs = now - lastLock;
    // Le dernier rouleau est le moment le plus tendu du tour : l'entrain
    // ne descend donc pas à zéro quand il n'en reste qu'un, il se
    // concentre. 100, 78, 62 pour trois rouleaux en vol, puis deux, puis un.
    d.energy = flying == 0 ? 0
                           : static_cast<uint8_t>(100 - 22 * (reels - flying));
    return d;
}

BandDrive bandDriveOfWin(uint8_t tier) {
    // Escalier de D-008 : la sobriété d'un petit gain est ce qui rend le
    // jackpot énorme. Un bandeau à fond pour deux cerises et il n'y a plus
    // rien à donner au-dessus.
    static const uint8_t kByTier[5] = {0, 35, 55, 78, 100};
    BandDrive d;
    d.energy = kByTier[tier < 5 ? tier : 4];
    return d;
}

uint8_t bandLevel(uint8_t bar, uint32_t now, const BandDrive& d) {
    return levelAtTick(bar, now / kBandStepMs, d);
}

uint8_t bandPeak(uint8_t bar, uint32_t now, const BandDrive& d) {
    // Maximum des six derniers paliers, soit un tiers de seconde de
    // mémoire — assez pour que le témoin plane, assez peu pour qu'il
    // retombe entre deux poussées.
    const uint32_t tick = now / kBandStepMs;
    uint8_t best = 0;
    for (uint32_t j = 0; j < 6; ++j) {
        if (j > tick) break;
        const uint8_t v = levelAtTick(bar, tick - j, d);
        if (v > best) best = v;
    }
    return best;
}

}  // namespace core
