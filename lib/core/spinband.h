// Le bandeau du bas pendant un tour : un analyseur de spectre, façon
// façade de juke-box. Il remplace un « SPINNING » figé qui laissait vingt
// pixels de haut sur toute la largeur ne rien raconter.
//
// C'est du MOUVEMENT, donc de la logique — il vit ici et pas dans le
// dessin, et le temps entre par `now`. Surtout, tout est **pur** : aucun
// état à faire avancer, aucune horloge interne. La hauteur d'une barre est
// une fonction de (barre, instant, entrain), et l'entrain se déduit des
// rouleaux eux-mêmes. Conséquences directes : les captures sont
// reproductibles, et un test n'a rien à remonter pour interroger une image
// précise.
#pragma once
#include <cstdint>

#include "reel_motion.h"

namespace core {

// 24 barres sur 240 pixels : un pas de 10, soit 8 de large et 2 de blanc.
// Assez fin pour onduler, assez gros pour se lire à 245 ppi.
constexpr uint8_t kBandBars = 24;

// Le spectre se recalcule par paliers, pas à chaque image : à 30 images
// par seconde, un tirage par image donne un grésillement, pas un rythme.
constexpr uint32_t kBandStepMs = 55;

// Ce qui pousse le bandeau. Déduit, jamais stocké.
struct BandDrive {
    uint8_t energy = 0;        // 0..100, l'entrain général
    uint32_t sinceLockMs = 0;  // temps depuis le dernier rouleau verrouillé
    bool locked = false;       // au moins un rouleau s'est déjà arrêté
};

// L'entrain lu directement dans les rouleaux : chacun connaît son instant
// d'arrêt (`t0 + dur`), donc le nombre de rouleaux encore en vol et la
// fraîcheur du dernier verrouillage se calculent sans rien mémoriser.
BandDrive bandDriveOfReels(const ReelMotion* m, uint8_t reels, uint32_t now);

// L'entrain d'une célébration. L'escalier de D-008 s'applique ici aussi :
// un petit gain ne doit pas faire le même bruit visuel qu'un jackpot,
// sinon le jackpot n'en fait plus.
BandDrive bandDriveOfWin(uint8_t tier);

// Hauteur d'une barre, 0..100. `bar` va de 0 à kBandBars-1.
uint8_t bandLevel(uint8_t bar, uint32_t now, const BandDrive& d);

// Le témoin de crête qui retombe lentement — c'est lui qui fait lire un
// analyseur de spectre plutôt qu'une rangée de barres agitées. Pas de
// mémoire non plus : c'est le maximum des dernières hauteurs, et comme
// bandLevel() est pure, on peut les redemander.
uint8_t bandPeak(uint8_t bar, uint32_t now, const BandDrive& d);

}  // namespace core
