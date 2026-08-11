// Blackjack — règles pures, testables sans écran.
//
// Règles retenues (les plus courantes, et les plus favorables au joueur
// parmi les variantes simples) :
//   - sabot de 4 jeux, remélangé à 25 % de pénétration restante ;
//   - le croupier tire jusqu'à 17 et RESTE sur 17 souple (S17) ;
//   - blackjack payé 3:2, égalité remboursée ;
//   - doublement autorisé sur les deux premières cartes uniquement ;
//   - pas de split ni d'assurance : deux règles de plus pour un écran de
//     240 px, ce serait payer cher une profondeur que personne ne verra.
#pragma once
#include <cstdint>

#include "cards.h"
#include "economy.h"
#include "rng.h"

namespace core {

enum class BjPhase : uint8_t {
    Idle,        // entre deux mains, en attente de mise
    PlayerTurn,  // le joueur tire, reste ou double
    DealerTurn,  // le croupier applique sa règle, carte par carte
    Settle,      // résultat affiché
};

enum class BjAction : uint8_t { Hit, Stand, Double };

enum class BjOutcome : uint8_t {
    None,
    PlayerBlackjack,  // 3:2
    PlayerWin,
    DealerWin,
    PlayerBust,
    DealerBust,
    Push,
};

constexpr uint8_t kDealerStandsOn = 17;

struct Blackjack {
    Shoe shoe;
    Hand player;
    Hand dealer;
    BjPhase phase = BjPhase::Idle;
    BjOutcome outcome = BjOutcome::None;
    uint16_t stake = 0;
    uint32_t payout = 0;
    bool doubled = false;
    bool bailedOut = false;
    uint32_t hands = 0;
};

// Distribue une main. Débite la mise. Renvoie false si le solde ne suffit
// pas (l'appelant renfloue). Peut conclure immédiatement sur un blackjack.
bool bjDeal(Blackjack& b, Economy& e, RngFn rng);

// Le doublement n'est offert que sur les deux premières cartes, et
// seulement si le solde peut encaisser la seconde mise.
bool bjCanDouble(const Blackjack& b, const Economy& e);

// Applique une action du joueur. Sans effet hors de PlayerTurn.
void bjAct(Blackjack& b, BjAction a, Economy& e, RngFn rng);

// Fait avancer le croupier d'UNE carte. Renvoie true tant qu'il joue —
// une carte par pas, pour que l'écran puisse les montrer une à une.
bool bjDealerStep(Blackjack& b, RngFn rng);

// Solde le coup : calcule l'issue, crédite, applique le renflouement.
void bjSettle(Blackjack& b, Economy& e);

// Gain rendu au joueur pour une issue donnée (mise incluse).
uint32_t bjPayoutFor(BjOutcome o, uint16_t stake);

}  // namespace core
