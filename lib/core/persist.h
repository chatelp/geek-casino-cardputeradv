// Sauvegarde — logique pure, UNE seule structure.
//
// Il y a eu un temps deux blocs : la sauvegarde principale et un bloc
// annexe pour les mises, séparé afin de ne pas invalider les classements
// existants en changeant de format. Pierre ayant accepté de repartir de
// zéro, tout est revenu dans une seule structure — un magic, une version,
// une somme de contrôle, un chemin de lecture et d'écriture.
//
// Le stockage (NVS appareil, fichier sim) vit
// dans les mains ; ici on décide de ce qui est sauvé et de ce qui rend une
// sauvegarde recevable : magie, version, somme de contrôle, bornes. Une
// sauvegarde douteuse est REJETÉE en bloc, jamais rafistolée à moitié.
#pragma once
#include <cstdint>

#include "players.h"

namespace core {

constexpr uint32_t kSaveMagic = 0x47434133;  // "GCA3"
constexpr uint16_t kSaveVersion = 3;

enum class Skin : uint8_t { Geek = 0, Classic = 1 };

// Délais de déclenchement du mode démo, en secondes. Une échelle plutôt
// qu'une saisie libre : on change d'un cran, sans clavier numérique.
constexpr uint16_t kDemoDelays[] = {10, 20, 30, 60, 120, 300};
constexpr uint8_t kDemoDelaySteps = sizeof(kDemoDelays) / sizeof(kDemoDelays[0]);
constexpr uint8_t kDefaultDemoDelay = 1;  // 20 s

struct Settings {
    uint8_t volume = 1;   // 0..3 — défaut BAS : l'objet ne crie pas (D-014)
    uint8_t muted = 0;
    uint8_t slotSkin = 0; // Skin — uint8 pour une taille de struct stable
    // Le mode démo est une invitation, pas une obligation : sur un bureau
    // partagé, une machine qui se lance seule peut déranger.
    uint8_t demoOn = 1;
    uint8_t demoDelay = 1;  // indice dans kDemoDelays, 20 s
    // Séquence d'allumage façon vieille borne. Charmante une fois,
    // pénible si on flashe vingt fois par heure : elle se coupe.
    uint8_t bootFx = 1;
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

}  // namespace core
