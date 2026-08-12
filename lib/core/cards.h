// Cartes et sabot — logique pure, partagée par tous les jeux de cartes.
#pragma once
#include <cstdint>

#include "rng.h"

namespace core {

constexpr uint8_t kDecks = 4;                     // sabot de 4 jeux
constexpr uint16_t kShoeSize = 52 * kDecks;
constexpr uint8_t kHandMax = 12;                  // au-delà, on a forcément sauté

// rank 1..13 (1 = As, 11 = Valet, 12 = Dame, 13 = Roi), suit 0..3.
struct Card {
    uint8_t rank;
    uint8_t suit;
};

struct Hand {
    Card c[kHandMax];
    // Valeur par défaut OBLIGATOIRE : sans elle, une main déclarée sans
    // initialisation part avec un compte tiré de la pile mémoire, et
    // l'écran dessine des cartes qui n'existent pas. Bug vu sur appareil.
    uint8_t n = 0;
};

struct HandValue {
    uint8_t total;
    bool soft;  // un As compte encore 11 : la main peut encaisser sans sauter
};

void handClear(Hand& h);
void handAdd(Hand& h, Card card);

// Meilleur total ne dépassant pas 21 si possible. Un As vaut 11 tant que
// ça ne fait pas sauter, 1 ensuite — c'est TOUTE la subtilité du jeu, elle
// est ici et nulle part ailleurs.
HandValue handValue(const Hand& h);

bool isBlackjack(const Hand& h);  // 21 en exactement deux cartes
bool isBust(const Hand& h);

// Sabot mélangé. `pos` avance ; le sabot se remélange tout seul quand il
// approche de la fin (pénétration ~75 %).
struct Shoe {
    uint8_t card[kShoeSize];  // index 0..51 : rank = i%13+1, suit = i/13
    uint16_t pos = 0;
    // `false` par défaut : un sabot qui se prétendrait prêt distribuerait
    // depuis un tableau jamais mélangé.
    bool ready = false;
};

// Jeu UNIQUE de 52 cartes. Le video poker en dépend : ses probabilités
// et son taux de retour sont calculés sur un seul jeu, jamais sur un
// sabot. Distribuer d'un sabot de quatre jeux changerait le jeu en
// silence — quatre rois possibles au lieu de quatre au total.
struct Deck {
    uint8_t card[52];
    uint8_t pos = 0;
};

void shuffleDeck(Deck& d, RngFn rng);
Card dealFromDeck(Deck& d);
uint8_t deckLeft(const Deck& d);

void shuffleShoe(Shoe& s, RngFn rng);
Card dealCard(Shoe& s, RngFn rng);
uint16_t cardsLeft(const Shoe& s);
bool needsShuffle(const Shoe& s);

}  // namespace core
