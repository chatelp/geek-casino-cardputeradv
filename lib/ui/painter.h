// Primitives de dessin : texte bitmap et glyphes indexés.
// Uniquement l'API lgfx:: — rien de matériel, donc identique en simulateur
// et sur l'appareil.
#pragma once
#include <cstdint>

#include "font5x7.h"
#include "scope.h"
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

// Chevron vertical. La fonte 5x7 n'a pas de flèche, et écrire « V » pour
// dire « bas » se lit comme la lettre V : il faut le dessiner.
void drawChevronV(lgfx::LGFX_Sprite& g, int x, int y, bool down, uint16_t color);

// Piste de circuit entre deux points. Une piste ne tourne JAMAIS à 90°
// sur une vraie carte : elle casse l'angle à 45°. C'est le détail qui
// distingue un dessin de circuit d'un simple quadrillage.
void trace45(lgfx::LGFX_Sprite& g, int x0, int y0, int x1, int y1,
             uint16_t color, int w = 1);

// Via : anneau métallisé percé au centre.
void drawVia(lgfx::LGFX_Sprite& g, int x, int y, uint16_t ring, uint16_t hole);

// Dos de carte 14x20, mis à l'échelle.
void drawCardBack(lgfx::LGFX_Sprite& g, int x, int y, int scale = 2);
void drawCardBackGray(lgfx::LGFX_Sprite& g, int x, int y, int scale = 2);

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

// Passe TOUT le sprite en niveaux de gris, une fois, après le dessin.
//
// Le mode démo était grisé couleur par couleur, à l'appel : chaque écran
// portait ses aides A()/D(), et tout ce qu'on dessinait ensuite en les
// oubliant restait en couleurs — le décor de circuit imprimé du poker, son
// curseur, son bouton DRAW. Une règle qu'il faut se rappeler d'appliquer à
// chaque trait finit toujours par être oubliée quelque part.
//
// Ici la conversion porte sur le résultat, pas sur l'intention : rien de ce
// qui est à l'écran ne peut y échapper, y compris ce qu'on dessinera plus
// tard sans y penser.
void desaturate(lgfx::LGFX_Sprite& g);

// La trace d'oscilloscope du bas d'écran, pendant un tour. Les écarts
// viennent de core::scopeAt() — ici on ne fait que peindre. La trace est
// dessinée CONTINUE (chaque colonne rejoint la précédente) : une suite de
// points isolés ne se lit pas comme un signal.
void drawScope(lgfx::LGFX_Sprite& g, int x, int y, int w, int h,
               const core::ScopeDrive& d, uint32_t now);

}  // namespace ui
