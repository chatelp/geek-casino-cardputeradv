// Session de blackjack : les règles (blackjack.h) plus le rythme et le
// choix d'action. Logique pure, temps injecté.
#pragma once
#include <cstdint>

#include "blackjack.h"
#include "game.h"  // Cue
#include "rng.h"

namespace core {

// Ce que le joueur peut choisir à l'écran. L'ordre est celui d'affichage.
enum class BjChoice : uint8_t { Hit = 0, Stand = 1, Double = 2 };
constexpr uint8_t kBjChoices = 3;

struct BjSession {
    Blackjack bj;
    Economy econ;

    BjChoice choice = BjChoice::Hit;
    uint32_t phaseT0 = 0;
    uint32_t lastStepMs = 0;   // dernière carte donnée au croupier
    uint32_t lastInputMs = 0;
    uint8_t revealed = 0;      // cartes déjà montrées (distribution animée)
    bool hintsOn = false;      // réglage propre au jeu
    // Tour joué par la machine : gratuit, muet, et rendu en gris.
    bool attract = false;
    uint32_t hands = 0;

    Cue cueQueue[6] = {};
    uint8_t cueHead = 0, cueTail = 0;
};

// Cadences : assez lentes pour se lire, assez vives pour ne pas lasser.
constexpr uint32_t kBjDealStepMs = 260;    // une carte à la distribution
constexpr uint32_t kBjDealerStepMs = 620;  // une carte du croupier
constexpr uint32_t kBjSettleMs = 2600;     // résultat affiché

BjSession newBjSession(uint32_t now);

bool bjStartHand(BjSession& s, uint32_t now, RngFn rng, bool byPlayer = true);
void bjMoveChoice(BjSession& s, int8_t delta, uint32_t now);
void bjConfirm(BjSession& s, uint32_t now, RngFn rng);
void bjUpdate(BjSession& s, uint32_t now, RngFn rng);

// Nombre de cartes effectivement visibles (distribution progressive).
uint8_t bjVisiblePlayer(const BjSession& s);
uint8_t bjVisibleDealer(const BjSession& s);
// La deuxième carte du croupier reste cachée jusqu'à son tour.
bool bjHoleHidden(const BjSession& s);

// Stratégie de base simplifiée (sans split ni assurance) : ce que le
// réglage HINTS conseille. Testée contre la table de référence.
BjAction bjBasicStrategy(const Hand& player, Card dealerUp, bool canDouble);

void pushBjCue(BjSession& s, Cue c);
Cue takeBjCue(BjSession& s);

}  // namespace core
