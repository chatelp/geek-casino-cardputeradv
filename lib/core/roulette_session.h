// Session de roulette : choix de la mise, lancer, arrêt, règlement.
// Le mouvement réutilise ReelMotion — une bille qui ralentit et dépasse
// d'un cran, c'est exactement la courbe déjà testée des rouleaux.
#pragma once
#include <cstdint>

#include "economy.h"
#include "game.h"  // Cue, Phase
#include "reel_motion.h"
#include "roulette.h"
#include "rng.h"

namespace core {

enum class RltPhase : uint8_t { Idle, Spinning, Result };

// La bille tourne trois fois plus longtemps qu'un rouleau : c'est le
// suspense propre au jeu, et il ne coûte rien puisque le joueur a déjà
// tout décidé avant de lancer.
constexpr uint32_t kRltSpinMs = 3200;
constexpr uint32_t kRltResultMs = 2600;

struct RouletteSession {
    Economy econ;
    BetKind kind = BetKind::Red;
    uint8_t straight = 0;     // numéro joué en plein
    uint8_t restIndex = 0;    // case affichée au repos
    ReelMotion motion;

    uint8_t winNumber = 0;
    RltPhase phase = RltPhase::Idle;
    uint16_t stake = 0;
    uint32_t payout = 0;
    bool won = false;
    bool attract = false;     // tour de démo : gratuit, muet, gris
    bool bailedOut = false;
    uint32_t phaseT0 = 0;
    uint32_t spins = 0;

    Cue cueQueue[6] = {};
    uint8_t cueHead = 0, cueTail = 0;
};

RouletteSession newRouletteSession(uint32_t now, RngFn rng);

void rltCycleBet(RouletteSession& s, int8_t delta);
void rltCycleNumber(RouletteSession& s, int8_t delta);
bool rltSpin(RouletteSession& s, uint32_t now, RngFn rng, bool byPlayer = true);
void rltUpdate(RouletteSession& s, uint32_t now);

// Position fractionnaire sur la roue, pour l'affichage.
float rltWheelPos(const RouletteSession& s, uint32_t now);

void pushRltCue(RouletteSession& s, Cue c);
Cue takeRltCue(RouletteSession& s);

}  // namespace core
