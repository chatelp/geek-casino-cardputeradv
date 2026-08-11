// Table de gains UNIQUE, partagée par tous les formats de machine à sous.
//
// Une seule forme : pay[symbole][nombre d'alignés]. Le 3×1 y range ses
// valeurs en [2] et [3], le format vidéo en [3], [4] et [5]. Avant cette
// unification il y avait deux structures disant la même chose, donc deux
// endroits à modifier pour un seul réglage — et rien qui casse si on en
// oublie un. C'est ce silence-là qu'on a supprimé.
//
// Ce qui reste volontairement distinct : les BANDES. Le format vidéo a la
// sienne parce que sur cinq rouleaux le jackpot serait introuvable avec
// celle du 3×1 (voir multiline.cpp). C'est un choix de jeu, pas un doublon.
#pragma once
#include <cstdint>

#include "reels.h"
#include "symbol_ids.h"

namespace core {

struct Paytable {
    // L'index EST le nombre de symboles identiques depuis la gauche.
    // pay[s][0] et pay[s][1] valent toujours 0 : un symbole seul ne paie
    // jamais, quel que soit le format.
    uint16_t pay[kSymbolCount][kMaxReels + 1];
};

// Les deux calibrages. Même structure, bandes et longueurs différentes.
const Paytable& mvpPaytable();    // 3 rouleaux, 1 ligne
const Paytable& videoPaytable();  // 5 rouleaux, 5 lignes

struct LineWin {
    uint8_t line;         // 0 pour un jeu à ligne unique
    uint8_t symbol;
    uint8_t count;        // 0 = ligne perdante
    uint16_t multiplier;
    bool jackpot;         // ligne pleine du symbole jackpot
};

// Évalue UNE ligne, lue depuis la gauche. C'est la seule fonction de
// calcul de gain du projet : le 3×1 l'appelle une fois, le 5×3 cinq fois.
LineWin evaluateLine(const Paytable& pt, const uint8_t* sym, uint8_t reels,
                     uint8_t line = 0);

// Espérance exacte d'une ligne, en multiplicateurs de la mise engagée sur
// elle. Calcul analytique : l'énumération des combinaisons est inutile
// puisque la probabilité d'aligner k symboles ne dépend que des effectifs
// des bandes. Vaut pour 3 comme pour 5 rouleaux, bandes différentes
// comprises.
double exactLineRtp(const ReelSet& rs, const Paytable& pt, uint8_t reels);
double exactLineHitRate(const ReelSet& rs, const Paytable& pt, uint8_t reels);

}  // namespace core
