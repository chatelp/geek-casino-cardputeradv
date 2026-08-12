// Mouvement d'un rouleau dans le temps — logique pure, donc testable.
//
// Le rythme est de la logique, pas du dessin : la courbe de décélération,
// le dépassement et l'instant d'arrêt se vérifient par des tests, sans
// écran. lib/ui ne fait qu'afficher la position que ce fichier calcule.
#pragma once
#include <cstdint>

namespace core {

// Cadence visée. Sert aux durées, pas au dessin.
constexpr uint32_t kFrameMs = 33;  // ~30 images/s

// Cascade d'arrêt : le premier rouleau se pose tôt, le dernier tard.
// L'attente sur le dernier est le seul suspense que la machine possède.
constexpr uint32_t kSpinBaseMs = 900;
constexpr uint32_t kSpinStaggerMs = 450;

// Dépassement à l'arrêt, en symboles. Sans lui le rouleau « se pose » ;
// avec lui il « claque ». Le son d'arrêt tombe sur le pic, pas sur
// l'immobilisation.
constexpr float kOvershootSymbols = 0.55f;
constexpr float kOvershootStart = 0.82f;  // fraction de la durée

struct ReelMotion {
    float startPos = 0;   // position sur la bande au départ
    float travel = 0;     // distance parcourue, en symboles (entier)
    uint32_t t0 = 0;      // instant de départ, ms
    uint32_t dur = 1;     // durée, ms
};

// Arme un rouleau : il partira de `from` et s'arrêtera exactement sur
// `to`, après au moins `minTurns` tours complets de bande.
ReelMotion armReel(uint32_t now, uint8_t reel, uint16_t from, uint16_t to,
                   uint16_t stripLen, uint8_t minTurns);

// Même courbe, durée choisie. Une bille de roulette tourne bien plus
// longtemps qu'un rouleau : la durée fait partie du jeu, pas du décor.
ReelMotion armReelMs(uint32_t now, uint16_t from, uint16_t to,
                     uint16_t stripLen, uint8_t minTurns, uint32_t durMs);

// Position fractionnaire sur la bande. La partie entière donne le symbole
// affiché, la partie fractionnaire le décalage en pixels.
float reelPosition(const ReelMotion& m, uint32_t now);

// Vrai dès que le rouleau est immobile.
bool reelSettled(const ReelMotion& m, uint32_t now);

// Instant exact où le rouleau touche son arrêt — c'est là que le son
// d'arrêt doit être déclenché.
inline uint32_t reelStopMs(const ReelMotion& m) { return m.t0 + m.dur; }

}  // namespace core
