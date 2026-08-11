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

// Joue un tour. `charge` = false pour le mode démo : le tirage et
// l'affichage sont réels mais AUCUN jeton ne bouge — une démo qui mise
// l'argent du joueur serait une faute, pas une animation.
bool playSpin(Machine& m, RngFn rng, SpinOutcome& out, bool charge = true);

}  // namespace core
