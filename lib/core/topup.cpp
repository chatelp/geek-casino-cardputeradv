#include "topup.h"

namespace core {

namespace {

// Le même hachage que l'allumage et l'oscilloscope : du bruit qui ne
// dépend que de ses entrées, donc des captures reproductibles.
uint32_t hash(uint32_t a, uint32_t b) {
    uint32_t h = a * 2654435761u ^ b * 2246822519u;
    h ^= h >> 13;
    h *= 3266489917u;
    h ^= h >> 16;
    return h;
}

constexpr int16_t kScreenW = 240;
constexpr int16_t kScreenH = 135;

}  // namespace

int32_t topupAmount(uint8_t digit) {
    if (digit == 0) return 100;
    if (digit > 9) return 0;
    return static_cast<int32_t>(digit) * 10;
}

float topupProgress(uint32_t t0, uint32_t now) {
    if (now <= t0) return 0.0f;
    const uint32_t e = now - t0;
    return e >= kTopupFxMs ? 1.0f : static_cast<float>(e) / kTopupFxMs;
}

bool topupCoinAt(uint8_t coin, uint32_t age, int16_t* x, int16_t* y,
                 uint8_t* scale) {
    const uint32_t h = hash(coin, 0x70FFu);
    // Départs étalés sur la première moitié : la pluie s'installe au lieu
    // de tomber d'un bloc — un rideau unique ne se lit pas comme une pluie.
    const uint32_t delay = (h % 800u);
    if (age < delay) return false;
    const uint32_t t = age - delay;

    // Chute accélérée, ~650 ms de haut en bas. La pièce traverse et sort :
    // elle ne s'empile pas, l'écran doit rester lisible derrière.
    const uint32_t fall = 560u + ((h >> 8) % 240u);
    if (t >= fall) return false;
    const float p = static_cast<float>(t) / fall;

    // x : colonne tirée du hachage, plus un balancement qui s'amortit —
    // une pièce tombe en oscillant, pas en ligne droite.
    const int16_t col = static_cast<int16_t>((h >> 4) % (kScreenW - 24)) + 6;
    const float swayHz = 2.0f + ((h >> 16) & 3u);
    const float phase = p * swayHz * 6.2832f;
    // sin approché par triangle : pas de libm dans la logique pure.
    const float k = phase - static_cast<int>(phase / 6.2832f) * 6.2832f;
    const float tri = k < 3.1416f ? (k / 1.5708f - 1.0f) : (3.0f - k / 1.5708f);
    const int16_t sway = static_cast<int16_t>(tri * 9.0f * (1.0f - p));

    *x = static_cast<int16_t>(col + sway);
    // p² : la gravité accélère. Départ 14 px au-dessus, sortie 14 dessous.
    *y = static_cast<int16_t>(-14.0f + (kScreenH + 28.0f) * p * p);
    *scale = ((h >> 20) & 1u) ? 2 : 1;
    return *y > -14 && *y < kScreenH;
}

}  // namespace core
