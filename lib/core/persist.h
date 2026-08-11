// Sauvegarde — logique pure. Le stockage (NVS appareil, fichier sim) vit
// dans les mains ; ici on décide de ce qui est sauvé et de ce qui rend une
// sauvegarde recevable : magie, version, somme de contrôle, bornes. Une
// sauvegarde douteuse est REJETÉE en bloc, jamais rafistolée à moitié.
#pragma once
#include <cstdint>

#include "players.h"

namespace core {

constexpr uint32_t kSaveMagic = 0x47434132;  // "GCA2"
constexpr uint16_t kSaveVersion = 2;

enum class Skin : uint8_t { Geek = 0, Classic = 1 };

struct Settings {
    uint8_t volume = 1;   // 0..3 — défaut BAS : l'objet ne crie pas (D-014)
    uint8_t muted = 0;
    uint8_t slotSkin = 0; // Skin — uint8 pour une taille de struct stable
};

struct SaveData {
    uint32_t magic;
    uint16_t version;
    uint8_t playerCount;
    uint8_t currentPlayer;
    Settings settings;
    uint8_t pad;
    Player players[kMaxPlayers];
    uint32_t sum;
};

uint32_t saveChecksum(const SaveData& s);
SaveData makeSave(const Roster& r, const Settings& st);
bool saveValid(const SaveData& s);

// Applique une sauvegarde valide. Renvoie false et ne touche à rien si
// elle ne l'est pas — l'appelant repart sur un premier lancement.
bool applySave(const SaveData& s, Roster& r, Settings& st);

// --- Mémoire des mises : un cran par JOUEUR et par JEU.
// Deux joueurs n'ont pas le même appétit, et au format vidéo une mise de
// 5 en engage 25 : une mise commune changerait l'enjeu à leur insu.
//
// Bloc séparé et facultatif : le mettre dans SaveData imposerait de
// bumper la version, donc d'invalider les sauvegardes existantes et
// d'effacer le classement. Son absence rend simplement les mises par
// défaut, sans rien casser.
constexpr uint32_t kBetMagic = 0x47434232;  // "GCB2"
constexpr uint8_t kBetGames = 3;            // slots, vidéo, blackjack

struct BetMemory {
    uint32_t magic;
    uint8_t bet[kMaxPlayers][kBetGames];
    uint8_t sum;
};

BetMemory makeBets(const BetMemory& src);   // recalcule la somme
bool betsValid(const BetMemory& b);
BetMemory freshBets();

}  // namespace core
