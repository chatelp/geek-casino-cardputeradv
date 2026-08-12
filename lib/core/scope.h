// La trace d'oscilloscope du bas d'écran, pendant un tour.
//
// Au repos elle est presque **plate** — un tiers de la demi-hauteur,
// plafond explicite dans le code. C'est ce vide au-dessus qui rend les
// à-coups visibles : l'analyseur de spectre qui a précédé est mort de ne
// pas l'avoir, ses barres remplissant déjà toute la hauteur.
//
// Un verrouillage de rouleau ne fait pas voyager un paquet le long de la
// courbe — ça se lisait comme des diodes qui défilent. Il **brouille la
// courbe entière** : elle se déchire par blocs, grésille colonne par
// colonne, puis se recale. C'est une secousse, pas un objet qui passe.
//
// Elle est aussi plus juste ici que des barres : le cabinet est une carte
// électronique (D-009), et ce qu'on pose sous une carte, c'est une sonde.
//
// Comme le reste du mouvement dans ce projet, c'est de la logique et c'est
// **pur** : aucun état, aucune horloge interne. L'écart au trait médian
// est une fonction de (colonne, instant, ce qui pousse), et ce qui pousse
// se déduit des rouleaux. Captures reproductibles, tests colonne par
// colonne.
#pragma once
#include <cstdint>

#include "reel_motion.h"

namespace core {

// Cinq secousses au plus : un rouleau chacune, au format vidéo.
constexpr uint8_t kScopeShocks = 5;

// Vitesse de défilement du signal au repos : un pixel toutes les quatre
// millisecondes. Assez lent pour que l'œil suive la forme, assez rapide
// pour que la trace vive.
constexpr uint32_t kScopeMsPerPx = 4;

// Durée d'une secousse. Au-delà, le brouillage devient un état plutôt
// qu'un événement — et on ne distingue plus les rouleaux entre eux.
constexpr uint32_t kScopeShockMs = 380;

// Le brouillage se retire par blocs de cette largeur : la courbe se
// déchire en segments décalés, comme un balayage qui perd sa synchro.
constexpr int kScopeTearW = 9;

// Le brouillage se rejoue à cette cadence. Plus lent, il paraît figé ;
// plus rapide, il n'est plus qu'un flou.
constexpr uint32_t kScopeShockStepMs = 30;

struct ScopeDrive {
    uint8_t energy = 0;                   // 0..100, l'agitation au repos
    uint32_t shockMs[kScopeShocks] = {};  // instants à brouiller
    uint8_t shocks = 0;
};

// Ce que racontent les rouleaux : chacun connaît son instant d'arrêt
// (`t0 + dur`), donc chaque verrouillage devient une secousse, et le
// nombre de rouleaux encore en vol donne l'agitation de fond.
ScopeDrive scopeDriveOfReels(const ReelMotion* m, uint8_t reels, uint32_t now);

// Pendant une célébration, un train de secousses régulier. Leur NOMBRE
// suit le palier : c'est l'escalier de D-008 appliqué ici — un petit gain
// fait une secousse, un jackpot en fait cinq.
ScopeDrive scopeDriveOfWin(uint8_t tier, uint32_t now);

// Écart au trait médian pour la colonne `x` d'une trace large de `w`,
// exprimé en centièmes de demi-hauteur : -100 à +100.
int8_t scopeAt(int x, int w, uint32_t now, const ScopeDrive& d);

// Intensité du brouillage à cet instant, 0..100. Sert au dessin, qui
// change de couleur quand la trace décroche.
uint8_t scopeShock(uint32_t now, const ScopeDrive& d);

}  // namespace core
