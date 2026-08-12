// La trace d'oscilloscope du bas d'écran, pendant un tour.
//
// Elle remplace un analyseur de spectre qui ne pouvait pas marcher : pour
// qu'un à-coup se voie, il faut de la marge au-dessus du repos, et des
// barres assez grosses pour être lisibles occupaient déjà toute la
// hauteur. Le registre lui-même était en cause, pas son réglage.
//
// Une trace n'a pas ce défaut : au repos elle reste **plate**, à un tiers
// de hauteur, et un événement peut donc la faire exploser sur toute
// l'amplitude. Le contraste est structurel, pas obtenu à force de réglage.
// Elle est aussi plus juste ici : le cabinet est une carte électronique
// (D-009), et ce qu'on met sous une carte, c'est une sonde.
//
// Comme le reste du mouvement dans ce projet, c'est de la logique et c'est
// **pur** : aucun état, aucune horloge interne. L'écart au trait médian
// est une fonction de (colonne, instant, ce qui pousse), et ce qui pousse
// se déduit des rouleaux. Captures reproductibles, tests image par image.
#pragma once
#include <cstdint>

#include "reel_motion.h"

namespace core {

// Cinq salves au plus : un rouleau chacune, au format vidéo.
constexpr uint8_t kScopeBursts = 5;

// Vitesse de défilement : une milliseconde tous les quatre pixels, soit
// environ une seconde pour traverser l'écran. Assez lent pour qu'une salve
// se suive de l'œil, assez rapide pour que deux verrouillages ne se
// rattrapent pas.
constexpr uint32_t kScopeMsPerPx = 4;

// Largeur d'une salve, en pixels. Au-delà elle devient une bosse ; en
// deçà, un point qu'on rate.
constexpr int kScopeBurstW = 18;

struct ScopeDrive {
    uint8_t energy = 0;                    // 0..100, l'agitation au repos
    uint32_t burstMs[kScopeBursts] = {};   // instants à faire exploser
    uint8_t bursts = 0;
};

// Ce que racontent les rouleaux : chacun connaît son instant d'arrêt
// (`t0 + dur`), donc chaque verrouillage devient une salve, et le nombre
// de rouleaux encore en vol donne l'agitation de fond.
ScopeDrive scopeDriveOfReels(const ReelMotion* m, uint8_t reels, uint32_t now);

// Pendant une célébration, un train de salves régulier. Le NOMBRE de
// salves suit le palier : c'est l'escalier de D-008 appliqué ici — un
// petit gain fait une secousse, un jackpot en fait cinq.
ScopeDrive scopeDriveOfWin(uint8_t tier, uint32_t now);

// Écart au trait médian pour la colonne `x` d'une trace large de `w`,
// exprimé en centièmes de demi-hauteur : -100 à +100.
int8_t scopeAt(int x, int w, uint32_t now, const ScopeDrive& d);

}  // namespace core
