#include "topup_fx.h"

#include "layout.h"
#include "painter.h"
#include "palette.h"

namespace ui {

using layout::kScreenH;
using layout::kScreenW;
namespace P = pal;

// L'animation « ultra casino » : une pluie de jetons dorés qui traverse
// l'écran, et le montant qui grimpe en gros au centre. Deux signaux — la
// matière qui tombe, le chiffre qui monte — parce qu'un seul des deux ne
// raconte qu'une moitié de l'histoire.
void drawTopupFx(lgfx::LGFX_Sprite& g, const core::Topup& t, uint32_t now) {
    if (!t.active) return;
    const uint32_t age = now - t.t0;
    const float p = core::topupProgress(t.t0, now);

    // La pluie, derrière le compteur.
    for (uint8_t i = 0; i < core::kTopupCoins; ++i) {
        int16_t x, y;
        uint8_t sc;
        if (core::topupCoinAt(i, age, &x, &y, &sc)) {
            drawIcon(g, ICON_COIN, x, y, sc);
        }
    }

    // Le montant, en énorme — même décompte que les célébrations : il
    // grimpe puis se fige, le joueur doit avoir le temps de LIRE le total.
    char buf[12];
    const uint32_t counted =
        core::countedPayout(static_cast<uint32_t>(t.shown), p);
    std::snprintf(buf, sizeof(buf), "+%u", counted);
    const int cx = kScreenW / 2;
    const int cy = kScreenH / 2 - 12;
    // Halo d'encre : le chiffre doit rester lisible sur N'IMPORTE quel
    // écran — c'est un overlay, il ne choisit pas son fond.
    for (int dx = -2; dx <= 2; dx += 2) {
        for (int dy = -2; dy <= 2; dy += 2) {
            drawText(g, buf, cx + dx, cy + dy, P::ink900, 3, Align::Center);
        }
    }
    drawText(g, buf, cx, cy, P::yellow, 3, Align::Center);

    // La mention, petite et sous le chiffre : c'est la maison qui offre,
    // autant le dire — des jetons virtuels se donnent, ils ne se vendent
    // pas.
    drawText(g, "HOUSE CREDIT", cx + 1, cy + 27, P::ink900, 1, Align::Center);
    drawText(g, "HOUSE CREDIT", cx, cy + 26, P::amber, 1, Align::Center);
}

}  // namespace ui
