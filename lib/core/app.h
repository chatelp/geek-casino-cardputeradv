// L'application entière — écrans, navigation, réglages, joueurs — en
// logique pure. Les mains ne font que trois choses : donner le temps,
// donner les touches, afficher l'écran que ce fichier désigne.
//
// Clavier (décisions Pierre) :
//   accueil : ↑/↓ choisir, Enter/Espace jouer, S réglages généraux,
//             L classement, H aide du jeu sélectionné
//   jeu     : Espace/Enter tirer, ←/→ mise, H aide, S réglages du jeu,
//             Échap retour accueil
//   partout : Échap revient en arrière (et quitte, au simulateur, depuis
//             l'accueil)
#pragma once
#include <cstdint>

#include "game.h"
#include "persist.h"
#include "players.h"

namespace core {

enum class AppScreen : uint8_t {
    NameEntry,       // premier lancement, nouveau joueur, ou après reset
    Lobby,
    Slot,
    SlotHelp,        // H — correspondances geek ↔ classique et gains
    SlotSettings,    // S en jeu — glyphes geek/classique
    GlobalSettings,  // S à l'accueil — son, volume, joueur, reset
    Leaderboard,     // L à l'accueil
};

// Touches abstraites vues par l'app (l'ASCII de la saisie du nom passe par
// feedNameChar, pas par ici).
enum class AppKey : uint8_t {
    None, Up, Down, Left, Right, Confirm,  // Enter ou Espace
    Help, Settings, Board, Back,
};

struct NameEntry {
    char buf[kNameMax + 1] = {0};
    uint8_t len = 0;
    bool rosterFull = false;  // affiché quand l'ajout est refusé
};

struct App {
    Game game;
    Roster roster;
    Settings settings;
    AppScreen screen = AppScreen::NameEntry;
    NameEntry nameEntry;
    uint8_t lobbyIndex = 0;     // 0 = SLOTS, seuls autres = à venir
    uint8_t menuIndex = 0;      // ligne sélectionnée dans les réglages
    bool resetArmed = false;    // le reset demande une seconde pression
    bool quitRequested = false; // Échap depuis l'accueil (sim uniquement)
    bool dirty = false;         // une sauvegarde est due
};

App newApp(uint32_t now, RngFn rng);

// Après chargement d'une sauvegarde : saute la saisie du nom s'il existe
// déjà des joueurs, et branche l'économie sur le joueur courant.
void enterFromSave(App& a);

// Une touche. Fait tout : navigation, réglages, lancement des tours.
void handleKey(App& a, AppKey k, uint32_t now, RngFn rng);

// Saisie du nom : A-Z et 0-9 uniquement, 8 caractères au plus.
void feedNameChar(App& a, char c);
void nameBackspace(App& a);

// À appeler chaque image. Fait avancer le jeu et pousse les sons.
void tickApp(App& a, uint32_t now, RngFn rng);

// Nombre de lignes des menus (pour l'affichage et la navigation).
constexpr uint8_t kGlobalSettingsRows = 4;  // SOUND, VOLUME, PLAYER, RESET
constexpr uint8_t kSlotSettingsRows = 1;    // GLYPHS

}  // namespace core
