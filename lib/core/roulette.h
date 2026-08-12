// Roulette européenne — un seul zéro, 37 cases.
//
// Logique pure. Le fait notable : TOUTES les mises simples ont exactement
// le même taux de retour, 36/37 = 97,297 %. C'est une propriété du jeu,
// pas un réglage — et elle se vérifie exactement, mise par mise.
#pragma once
#include <cstdint>

#include "economy.h"
#include "rng.h"

namespace core {

constexpr uint8_t kPockets = 37;  // 0 à 36

// Ordre PHYSIQUE de la roue européenne, pas l'ordre numérique. C'est lui
// qui rend le ralenti crédible : les cases qui défilent sous la bille
// sont celles qu'on verrait vraiment passer.
uint8_t pocketAt(uint8_t index);
uint8_t indexOfPocket(uint8_t number);

bool isRed(uint8_t number);
inline bool isBlack(uint8_t n) { return n != 0 && !isRed(n); }

enum class BetKind : uint8_t {
    Red = 0, Black, Odd, Even, Low, High, Dozen1, Dozen2, Dozen3, Straight,
};
constexpr uint8_t kBetKinds = 10;

const char* betName(BetKind k);
// Multiplicateur TOTAL rendu au joueur, mise comprise : 2 pour une chance
// simple, 3 pour une douzaine, 36 pour un plein.
uint16_t roulettePayout(BetKind k);
bool betWins(BetKind k, uint8_t straightPick, uint8_t number);

// Retour exact d'une mise, calculé sur les 37 cases. Doit valoir 36/37
// pour toutes : si une seule diffère, la table est fausse.
double exactRouletteRtp(BetKind k, uint8_t straightPick);

}  // namespace core
