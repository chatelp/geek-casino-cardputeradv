// Joueurs et classement — logique pure.
//
// Le leaderboard EST la table des joueurs : chaque joueur y vit avec son
// solde persistant, ses tours et son meilleur gain. Changer de joueur,
// c'est changer d'entrée courante ; le classement est une simple lecture
// triée. Une seule structure, pas deux à garder synchrones.
#pragma once
#include <cstdint>

#include "economy.h"

namespace core {

constexpr uint8_t kMaxPlayers = 8;
constexpr uint8_t kNameMax = 8;  // caractères, hors terminateur

struct Player {
    char name[kNameMax + 1];
    int32_t credits;
    uint32_t spins;
    uint32_t bestWin;
};

struct Roster {
    Player players[kMaxPlayers];
    uint8_t count = 0;
    uint8_t current = 0;
};

// Ajoute un joueur (solde de départ) et le rend courant. Si le nom existe
// déjà — même casse — on BASCULE dessus au lieu de créer un doublon.
// Renvoie false si le roster est plein et le nom inconnu.
bool addOrSwitchPlayer(Roster& r, const char* name);

// Joueur courant. count == 0 → nullptr : la saisie du nom s'impose.
Player* currentPlayer(Roster& r);

// Indices des joueurs triés par solde décroissant (à solde égal, meilleur
// gain). `out` reçoit r.count valeurs.
void rankPlayers(const Roster& r, uint8_t* out);

// Remet le classement à zéro : tous les joueurs disparaissent. L'appelant
// repasse par la saisie du nom, comme au premier lancement.
void resetRoster(Roster& r);

// Recopie économie → joueur courant, et met à jour le meilleur gain.
void syncPlayer(Roster& r, const Economy& e, uint32_t spins, uint32_t lastPayout);

// Nom valide : 1 à 8 caractères parmi A-Z et 0-9.
bool nameValid(const char* name);

}  // namespace core
