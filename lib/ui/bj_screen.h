#pragma once
#include <cstdint>

#include "bj_session.h"
#include "hal_display.h"
#include "layout.h"

namespace ui {

void drawBjScreen(lgfx::LGFX_Sprite& g, const core::BjSession& s, uint32_t now);

// Une carte, réutilisée par la page d'aide.
// `demo` rend la carte en niveaux de gris, comme le reste du mode démo :
// une carte en couleurs au milieu d'un écran gris casserait le message.
// Gabarit d'une carte. Le blackjack et le video poker n'ont pas les mêmes
// contraintes — l'un en loge jusqu'à cinq par main sur deux mains, l'autre
// en montre cinq et rien d'autre — donc ils ne dessinent pas la même
// carte. Un seul rendu, deux gabarits : c'est la taille qui change, pas le
// dessin.
struct CardSize {
    int w, h;
    int rankScale;  // échelle du rang
    int suitScale;  // échelle de l'enseigne 8x8
};

constexpr CardSize kBjCard{bjlayout::kCardW, bjlayout::kCardH, 2, 1};
constexpr CardSize kVpCard{vplayout::kCardW, vplayout::kCardH, 3, 2};

void drawCard(lgfx::LGFX_Sprite& g, core::Card c, int x, int y, bool faceDown,
              bool demo, const CardSize& sz = kBjCard);

}  // namespace ui
