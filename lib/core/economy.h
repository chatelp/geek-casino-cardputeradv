// Solde, mise et renflouement — logique pure.
//
// Garde-fou du projet (D-004) : le joueur ne peut JAMAIS être définitivement
// ruiné. Quand il ne peut plus payer la plus petite mise, la maison remet au
// pot. C'est un jouet ; un cul-de-sac serait un défaut, pas une punition.
#pragma once
#include <cstdint>

namespace core {

// Échelle de mises. Le joueur monte et descend d'un cran, il ne saisit pas
// un nombre : une seule touche suffit et aucune valeur absurde n'est
// atteignable.
constexpr uint16_t kBetLadder[] = {1, 2, 5, 10, 25, 50};
constexpr uint8_t kBetSteps = sizeof(kBetLadder) / sizeof(kBetLadder[0]);
constexpr uint8_t kDefaultBetIndex = 2;  // 5 jetons

constexpr int32_t kStartingCredits = 1000;
constexpr int32_t kBailoutCredits = 500;

struct Economy {
    int32_t credits;
    uint8_t betIndex;
};

Economy freshEconomy();

inline uint16_t bet(const Economy& e) { return kBetLadder[e.betIndex]; }

// Monte / descend d'un cran, sans jamais dépasser ce que le solde permet.
void raiseBet(Economy& e);
void lowerBet(Economy& e);

// Ramène la mise au plus grand cran finançable. Appelé après chaque tour :
// un joueur qui s'appauvrit voit sa mise suivre au lieu d'être bloqué.
void clampBet(Economy& e);

bool canSpin(const Economy& e);
void placeBet(Economy& e);
void award(Economy& e, uint32_t amount);

// Vrai quand même la plus petite mise est hors de portée.
bool needsBailout(const Economy& e);
void bailout(Economy& e);

}  // namespace core
