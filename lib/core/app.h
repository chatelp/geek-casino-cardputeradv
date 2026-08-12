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

#include "bj_session.h"
#include "game.h"
#include "persist.h"
#include "players.h"
#include "video_game.h"
#include "vp_session.h"

namespace core {

enum class AppScreen : uint8_t {
    NameEntry,       // premier lancement, nouveau joueur, ou après reset
    Lobby,
    Slot,
    SlotHelp,        // H — correspondances geek ↔ classique et gains
    SlotSettings,    // S en jeu — glyphes geek/classique
    Video,           // 5×3, 5 lignes
    VideoHelp,
    VideoSettings,
    Blackjack,
    BjHelp,
    BjSettings,
    Poker,
    PokerHelp,
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

// Trois jeux, un solde commun. L'entrée de l'accueil pointe l'un d'eux ;
// c'est ce qui donne son sens au mot « casino ».
enum class GameId : uint8_t { Slots = 0, Video = 1, Blackjack = 2, Poker = 3 };
constexpr uint8_t kGameCount = 4;

struct App {
    Game game;
    VideoGame video;
    BjSession bj;
    VpSession poker;
    Roster roster;
    Settings settings;
    AppScreen screen = AppScreen::NameEntry;
    NameEntry nameEntry;
    uint8_t lobbyIndex = 0;     // 0 = SLOTS, seuls autres = à venir
    uint8_t menuIndex = 0;      // ligne sélectionnée dans les réglages
    // L'aide s'ouvre depuis l'accueil OU depuis le jeu : on revient
    // toujours d'où l'on vient, jamais vers un écran câblé en dur.
    AppScreen helpReturn = AppScreen::Lobby;
    // Les aides tiennent sur plusieurs pages : un écran de 240x135 ne peut
    // pas expliquer cinq lignes de paiement ET une table de gains.
    uint8_t helpPage = 0;
    // Le solde vit dans App : les trois jeux le partagent au lieu d'en
    // garder chacun une copie qui divergerait.
    Economy econ;
    // Mise par joueur et par jeu, persistée à côté du classement.
    BetMemory bets;
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
constexpr uint8_t kBjSettingsRows = 1;      // HINTS

// Nombre de pages de chaque aide.
uint8_t helpPageCount(AppScreen help);

// Synchronise le SOLDE (partagé) — pas la mise, qui appartient au jeu.
void pushEconomy(App& a);
void pullEconomy(App& a);

// La mise appartient au couple (joueur, jeu). Ces deux fonctions font le
// va-et-vient entre la table persistée et les trois jeux.
void loadPlayerBets(App& a);   // table → jeux
void storePlayerBets(App& a);  // jeux → table
uint16_t betOfGame(const App& a, GameId g);

}  // namespace core
