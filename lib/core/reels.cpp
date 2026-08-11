#include "reels.h"

namespace core {

namespace {

// Bande de 32 positions. Les effectifs fixent la fréquence de chaque
// symbole ; ils ont été choisis avec la table de gains pour viser un RTP
// d'environ 95 % (vérifié exactement par test/test_paytable).
//
//   RESISTOR 8   LED 7   CHIP 6   FLOPPY 4
//   GAMEPAD  3   CRT 2   D20  1   INVADER 1     = 32
//
// Les symboles sont entrelacés plutôt que groupés : deux positions voisines
// se ressemblent peu, ce qui rend le défilement lisible et évite qu'un arrêt
// « raté » d'un pixel change le résultat de façon invisible.
constexpr uint8_t kStrip[kStripLen] = {
    SYM_RESISTOR, SYM_LED,     SYM_CHIP,     SYM_RESISTOR,
    SYM_FLOPPY,   SYM_LED,     SYM_RESISTOR, SYM_GAMEPAD,
    SYM_CHIP,     SYM_RESISTOR, SYM_LED,     SYM_CRT,
    SYM_RESISTOR, SYM_CHIP,    SYM_FLOPPY,   SYM_LED,
    SYM_RESISTOR, SYM_GAMEPAD, SYM_CHIP,     SYM_LED,
    SYM_INVADER,  SYM_RESISTOR, SYM_FLOPPY,  SYM_CHIP,
    SYM_LED,      SYM_GAMEPAD, SYM_RESISTOR, SYM_CRT,
    SYM_CHIP,     SYM_FLOPPY,  SYM_D20,      SYM_LED,
};

const ReelSet kMvp = {
    kMvpReels,
    {kStrip, kStrip, kStrip, nullptr, nullptr},
    {kStripLen, kStripLen, kStripLen, 0, 0},
};

}  // namespace

const ReelSet& mvpReelSet() { return kMvp; }

uint8_t symbolAt(const ReelSet& rs, uint8_t reel, int32_t pos) {
    const int32_t n = static_cast<int32_t>(rs.len[reel]);
    int32_t p = pos % n;
    if (p < 0) p += n;  // le rouleau tourne aussi vers le haut
    return rs.strip[reel][p];
}

uint16_t countOn(const ReelSet& rs, uint8_t reel, uint8_t sym) {
    uint16_t n = 0;
    for (uint16_t i = 0; i < rs.len[reel]; ++i) {
        if (rs.strip[reel][i] == sym) ++n;
    }
    return n;
}

void spin(const ReelSet& rs, RngFn rng, uint16_t* pos, uint8_t* sym) {
    for (uint8_t r = 0; r < rs.reels; ++r) {
        const uint16_t p = static_cast<uint16_t>(drawBelow(rng, rs.len[r]));
        pos[r] = p;
        sym[r] = rs.strip[r][p];
    }
}

}  // namespace core
