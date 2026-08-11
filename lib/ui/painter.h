// Primitives de dessin : texte bitmap et glyphes indexés.
// Uniquement l'API lgfx:: — rien de matériel, donc identique en simulateur
// et sur l'appareil.
#pragma once
#include <cstdint>

#include "font5x7.h"
#include "hal_display.h"
#include "symbols.h"

namespace ui {

enum class Align : uint8_t { Left, Center, Right };

int textWidth(const char* s, int scale);

void drawText(lgfx::LGFX_Sprite& g, const char* s, int x, int y, uint16_t color,
              int scale = 2, Align a = Align::Left);

// Dessine un nombre sans passer par snprintf (chiffres tabulaires).
void drawNumber(lgfx::LGFX_Sprite& g, int32_t v, int x, int y, uint16_t color,
                int scale = 2, Align a = Align::Left);
int numberWidth(int32_t v, int scale);

// `tint` non nul remplace toutes les couleurs opaques du glyphe — sert aux
// silhouettes clignotantes et aux symboles éteints. `classic` bascule sur
// l'habillage traditionnel (même index, même gain).
void drawSymbol(lgfx::LGFX_Sprite& g, uint8_t sym, int x, int y, int scale,
                uint16_t tint = 0, bool classic = false);

void drawIcon(lgfx::LGFX_Sprite& g, uint8_t icon, int x, int y, int scale = 1,
              uint16_t tint = 0);

// Variantes « niveaux de gris » pour le mode démo : chaque couleur est
// remplacée par un des 3 gris de kSymbolPaletteGray (mappage par
// luminance, calculé par gen.py). Le dessin garde son volume.
void drawSymbolGray(lgfx::LGFX_Sprite& g, uint8_t sym, int x, int y, int scale,
                    bool classic = false);
void drawIconGray(lgfx::LGFX_Sprite& g, uint8_t icon, int x, int y, int scale = 1);

// Enseigne de carte 8x8 (blackjack).
void blitSuit(lgfx::LGFX_Sprite& g, uint8_t suit, int x, int y, int scale = 1,
              uint16_t tint = 0);

void drawFrame(lgfx::LGFX_Sprite& g, int x, int y, int w, int h, uint16_t c,
               int t = 1);

// Petits chevrons de part et d'autre d'une valeur réglable. Sans eux, rien
// n'indique au joueur que ←/→ changent la mise : le réglage existait mais
// restait invisible.
void drawBetArrows(lgfx::LGFX_Sprite& g, int leftX, int rightX, int y,
                   uint16_t color, bool canLower, bool canRaise);

}  // namespace ui
