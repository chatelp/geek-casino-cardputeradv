// Session de video poker : distribution, choix des cartes gardées, tirage.
// Logique pure, temps injecté — même discipline que les autres jeux.
#pragma once
#include <cstdint>

#include "cards.h"
#include "economy.h"
#include "game.h"  // Cue
#include "poker.h"
#include "rng.h"

namespace core {

enum class VpPhase : uint8_t {
    Idle,     // entre deux mains
    Holding,  // le joueur choisit ce qu'il garde
    Result,   // main finale affichée
};

// Le curseur parcourt les cinq cartes PUIS une case « DRAW ». Sans elle,
// il faudrait une touche de plus pour lancer l'échange — et le Cardputer
// n'en a pas de libre qui soit évidente.
constexpr uint8_t kVpDrawSlot = kPokerHandSize;
constexpr uint8_t kVpSlots = kPokerHandSize + 1;

constexpr uint32_t kVpDealStepMs = 130;  // les cinq cartes arrivent une à une
constexpr uint32_t kVpDrawStepMs = 160;  // les remplaçantes aussi

struct VpSession {
    Deck deck;
    Hand hand;
    bool held[kPokerHandSize] = {};
    uint8_t cursor = 0;
    VpPhase phase = VpPhase::Idle;
    PokerRank result = PokerRank::None;

    uint16_t stake = 0;
    uint32_t payout = 0;
    bool bailedOut = false;
    bool maxBet = false;      // la royale paie 800 au lieu de 250
    uint32_t hands = 0;

    Economy econ;
    uint32_t phaseT0 = 0;
    uint32_t lastStepMs = 0;
    uint8_t revealed = 0;     // cartes déjà montrées
    uint8_t replacing = 0;    // cartes déjà remplacées au tirage

    Cue cueQueue[6] = {};
    uint8_t cueHead = 0, cueTail = 0;
};

VpSession newVpSession(uint32_t now);

bool vpDeal(VpSession& s, uint32_t now, RngFn rng);
void vpMoveCursor(VpSession& s, int8_t delta);
// Enter : garde/relâche la carte sous le curseur, ou lance l'échange si le
// curseur est sur DRAW.
void vpConfirm(VpSession& s, uint32_t now, RngFn rng);
void vpUpdate(VpSession& s, uint32_t now, RngFn rng);

uint8_t vpVisible(const VpSession& s);
bool vpIsMaxBet(const Economy& e);

void pushVpCue(VpSession& s, Cue c);
Cue takeVpCue(VpSession& s);

}  // namespace core
