// Classement des mains de poker et table de gains « Jacks or Better ».
// Logique pure : c'est la partie qui se vérifie EXACTEMENT, en énumérant
// les 2 598 960 mains de cinq cartes d'un jeu unique.
#pragma once
#include <cstdint>

#include "cards.h"
#include "economy.h"

namespace core {

// Ordre croissant de valeur. `None` = moins qu'une paire de valets, ce
// qui ne paie rien : c'est ce seuil qui donne son nom au jeu.
enum class PokerRank : uint8_t {
    None = 0,
    JacksOrBetter,
    TwoPair,
    ThreeOfAKind,
    Straight,
    Flush,
    FullHouse,
    FourOfAKind,
    StraightFlush,
    RoyalFlush,
};
constexpr uint8_t kPokerRankCount = 10;
constexpr uint8_t kPokerHandSize = 5;

// Classe une main de cinq cartes. L'As compte haut ET bas pour la quinte :
// A-2-3-4-5 et 10-V-D-R-A sont toutes deux des quintes.
PokerRank rankHand(const Hand& h);

// Table 9/6 « full pay » — le barème de référence, celui qui donne les
// 99,5 % de retour du video poker bien joué.
// La quinte royale paie 250 en temps normal et **800 à la mise maximale** :
// c'est ce bonus qui fait tout l'intérêt de miser gros, et c'est la
// signature du jeu.
uint16_t pokerPayout(PokerRank r, bool maxBet);
const char* pokerRankName(PokerRank r);

}  // namespace core
