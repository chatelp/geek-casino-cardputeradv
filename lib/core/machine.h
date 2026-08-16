// Un tour complet : miser, tirer, évaluer, payer, renflouer si besoin.
// Logique pure — c'est cette fonction que le mode démo et les tests
// statistiques appellent en boucle, sans rien afficher.
#pragma once
#include <cstdint>

#include "economy.h"
#include "paytable.h"
#include "reels.h"
#include "rng.h"

namespace core {

struct SpinOutcome {
    uint16_t pos[kMaxReels];
    uint8_t sym[kMaxReels];
    uint16_t stake;    // mise engagée
    uint32_t payout;   // jetons rendus (0 = perdu)
    LineWin win;
    bool bailedOut;    // la maison a remis au pot après ce tour
};

struct Machine {
    const ReelSet* reels;
    const Paytable* pay;
    Economy econ;
};

Machine mvpMachine();

// Un tour se joue en DEUX TEMPS, et l'ordre est le jeu lui-même : la mise
// part à l'appui, le gain n'arrive qu'à l'arrêt des rouleaux.
//
// Les faire ensemble a été un vrai bug de la 1.0, signalé vidéo à l'appui
// par un joueur : 965 jetons, mise de 5, gain de 10. Il attendait
// 965 → 960 → 970 et voyait 965 → 970 d'un bloc, à l'appui. Le solde
// contenait donc déjà le gain quand la célébration l'annonçait, et le tour
// paraissait n'avoir rien payé. Personne ne l'avait vu parce qu'on
// regardait tous les rouleaux, pas le compteur.
//
// `charge` = false pour le mode démo : le tirage et l'affichage sont réels
// mais AUCUN jeton ne bouge — une démo qui mise l'argent du joueur serait
// une faute, pas une animation.

// Premier temps : prendre la mise et tirer. Le résultat est connu tout de
// suite (les rouleaux ne font que le montrer), mais rien n'est payé.
bool playSpin(Machine& m, RngFn rng, SpinOutcome& out, bool charge = true);

// Second temps : payer, et renflouer si le tour a vidé le solde. À appeler
// quand le dernier rouleau s'est arrêté.
void settleSpin(Machine& m, SpinOutcome& out, bool charge = true);

}  // namespace core
