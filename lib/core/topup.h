// Rachat de jetons — la rangée de chiffres, partout dans l'app.
//
// `1` ajoute 10 jetons, `2` 20, ... `9` 90, `0` 100. C'est un geste
// TRANSVERSAL : il marche à l'accueil, dans les cinq jeux, dans les
// réglages — partout sauf là où les chiffres veulent dire autre chose
// (la saisie du nom) et pendant l'allumage (où toute touche saute
// l'intermède). Pas de menu, pas de confirmation : des jetons virtuels
// se donnent, ils ne se vendent pas — le cérémonial est dans
// l'ANIMATION, pas dans le parcours.
//
// L'animation est une pluie de jetons : chaque pièce est une fonction
// pure de (indice, âge) — position tirée d'un hachage, chute accélérée,
// balancement. Aucun état par pièce, donc rien à faire avancer, et les
// captures sont reproductibles image par image. Même architecture que la
// célébration, l'oscilloscope et le bruit d'allumage.
#pragma once
#include <cstdint>

namespace core {

// L'animation dure le temps de lire le montant — et les pressions
// s'empilent : marteler `0` cumule les +100 dans le même panneau.
constexpr uint32_t kTopupFxMs = 1600;

// Assez de pièces pour une pluie, pas assez pour un mur : l'écran de
// jeu doit rester reconnaissable derrière.
constexpr uint8_t kTopupCoins = 16;

struct Topup {
    int32_t shown = 0;    // cumul affiché pendant l'animation en cours
    uint32_t t0 = 0;
    bool active = false;
};

// Montant d'une touche : chiffre 1..9 → 10..90, 0 → 100.
int32_t topupAmount(uint8_t digit);

// Avancement 0..1 de l'animation.
float topupProgress(uint32_t t0, uint32_t now);

// Une pièce de la pluie. Renvoie false si elle n'est pas à l'écran à cet
// âge (pas encore partie, ou déjà tombée). x/y en pixels, scale 1 ou 2.
bool topupCoinAt(uint8_t coin, uint32_t age, int16_t* x, int16_t* y,
                 uint8_t* scale);

}  // namespace core
