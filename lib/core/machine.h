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
    WinResult win;
    bool bailedOut;    // la maison a remis au pot après ce tour
};

struct Machine {
    const ReelSet* reels;
    const Paytable* pay;
    Economy econ;
};

Machine mvpMachine();

// Joue un tour. Renvoie false sans rien changer si le solde ne permet même
// pas la plus petite mise ET que le renflouement n'a pas encore eu lieu —
// appeler bailout() d'abord dans ce cas (playSpin le fait automatiquement).
bool playSpin(Machine& m, RngFn rng, SpinOutcome& out);

}  // namespace core
