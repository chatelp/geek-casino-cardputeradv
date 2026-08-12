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

// Landing : la bille est arrivée mais rien n'est réglé — elle rebondit
// dans sa case pendant que la roue est déjà immobile. Sans cette phase,
// le son de gain claquait à l'image exacte où la bille se posait :
// l'arrêt était trop franc, il manquait le petit chaos de fin de course.
enum class RltPhase : uint8_t { Idle, Spinning, Landing, Result };

// La bille tourne trois fois plus longtemps qu'un rouleau : c'est le
// suspense propre au jeu, et il ne coûte rien puisque le joueur a déjà
// tout décidé avant de lancer.
constexpr uint32_t kRltSpinMs = 3200;
// Le rebond : deux allers-retours amortis autour de la case, puis plus
// rien. Assez long pour se voir, assez court pour ne pas retarder un
// résultat que le joueur connaît déjà des yeux.
constexpr uint32_t kRltLandMs = 520;
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
    // Dernière case franchie : sert à cliqueter une fois par case, et à
    // espacer les clics quand la bille va trop vite pour être suivie.
    int32_t lastTickPocket = -1;
    uint32_t lastTickMs = 0;

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
