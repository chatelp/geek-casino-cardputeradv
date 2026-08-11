// Aléa injecté — logique pure, aucun include Arduino/M5.
// Côté appareil : esp_random (TRNG matériel). Côté sim/test : PRNG
// déterministe à graine fixe. Toute la logique de jeu tire via RngFn.
#pragma once
#include <cstdint>

namespace core {

using RngFn = uint32_t (*)();

// Tirage uniforme dans [0, n) sans biais de modulo (rejet, méthode
// arc4random_uniform). n == 0 ou 1 renvoie 0.
uint32_t drawBelow(RngFn rng, uint32_t n);

// PRNG déterministe (xorshift32) pour sim et tests. État global par
// choix : RngFn est un simple pointeur de fonction, sans contexte.
void     seedXorShift(uint32_t seed);
uint32_t xorShift32();

}  // namespace core
