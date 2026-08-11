// Table de gains et évaluation d'une ligne — logique pure.
//
// Le taux de retour au joueur (RTP) naît de la table ET de la bande. Il se
// calcule EXACTEMENT par énumération des combinaisons (voir exactRtp), pas
// par simulation : c'est un nombre, pas une estimation.
#pragma once
#include <cstdint>

#include "reels.h"
#include "symbol_ids.h"

namespace core {

struct Paytable {
    // Multiplicateur de la mise pour 3 symboles identiques, par symbole.
    uint16_t three[kSymbolCount];
    // Multiplicateur pour 2 symboles identiques à partir de la gauche.
    // Uniforme : le petit gain sert à entretenir le rythme, pas à récompenser
    // un symbole en particulier.
    uint16_t two;
};

const Paytable& mvpPaytable();

struct WinResult {
    uint16_t multiplier;  // 0 = tour perdant
    uint8_t matched;      // 0, 2 ou 3 symboles alignés
    uint8_t symbol;       // symbole gagnant (indéfini si matched == 0)
    bool jackpot;         // 3 invaders
};

// Évalue la ligne de paiement. `sym` contient `reels` symboles, gauche à
// droite. Les combinaisons se lisent depuis la gauche, comme sur une
// machine réelle : 3 identiques, sinon 2 identiques en tête, sinon rien.
WinResult evaluate(const Paytable& pt, const uint8_t* sym, uint8_t reels);

// RTP exact, par énumération de toutes les combinaisons de la bande.
// Coût : produit des longueurs de bande (32^3 = 32768 pour le MVP).
double exactRtp(const ReelSet& rs, const Paytable& pt);

// Probabilité exacte qu'un tour rapporte quelque chose.
double exactHitRate(const ReelSet& rs, const Paytable& pt);

}  // namespace core
