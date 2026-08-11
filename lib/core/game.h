// Machine à états d'un tour — logique pure, sans écran ni horloge propre :
// le temps entre par `now`, ce qui rend chaque transition testable.
#pragma once
#include <cstdint>

#include "machine.h"
#include "reel_motion.h"
#include "rng.h"
#include "sound.h"

namespace core {

enum class Phase : uint8_t {
    Idle,       // en attente d'un geste
    Spinning,   // rouleaux en mouvement, cascade d'arrêt
    Celebrate,  // gain présenté, durée fonction du palier
    Bailout,    // la maison remet au pot
};

// Escalier d'effets (D-008). La sobriété des tours ordinaires est ce qui
// rend le jackpot énorme — un palier de plus, c'est un palier de moins
// pour tous les autres.
enum class Tier : uint8_t {
    None = 0,   // perte : aucun effet
    Small = 1,  // 2 identiques, ou petit trois-de-suite
    Mid = 2,
    Big = 3,
    Jackpot = 4,
};

Tier tierOf(const LineWin& w);
uint32_t celebrateMs(Tier t);

// Le mode démo prend la main après ce délai d'inactivité (D-005).
constexpr uint32_t kAttractDelayMs = 12000;
// ...puis respire entre deux tours. Sans cette pause, la démo enchaînait
// sans répit : l'inactivité restait vraie, donc elle relançait aussitôt.
constexpr uint32_t kAttractIntervalMs = 5000;

struct Game {
    Machine machine;
    SpinOutcome outcome;
    ReelMotion motion[kMaxReels];
    uint16_t restPos[kMaxReels];  // position affichée hors mouvement
    Phase phase = Phase::Idle;
    Tier tier = Tier::None;
    uint32_t phaseT0 = 0;
    uint32_t lastInputMs = 0;
    bool attract = false;         // le tour courant est joué par la machine
    uint8_t reelsStopped = 0;
    uint32_t lastAttractMs = 0;
    uint32_t spins = 0;           // compteur de session, persisté

    // File de signaux sonores. La logique dit QUOI jouer et QUAND ; c'est
    // lib/hal qui sait comment. Une file évite de perdre un son quand deux
    // événements tombent sur la même image.
    Cue cueQueue[6] = {};
    uint8_t cueHead = 0, cueTail = 0;
};

void pushCue(Game& g, Cue c);
Cue takeCue(Game& g);  // Cue::None quand la file est vide

Game newGame(uint32_t now, RngFn rng);

// Lance un tour. Sans effet si un tour est déjà en cours ou si le solde
// l'interdit. `byPlayer` distingue un geste du joueur du mode démo.
bool startSpin(Game& g, uint32_t now, RngFn rng, bool byPlayer = true);

// Fait avancer l'état. Renvoie le nombre de rouleaux qui viennent de
// s'arrêter pendant cette image — c'est le signal de déclenchement du son.
uint8_t updateGame(Game& g, uint32_t now, RngFn rng);

// Position fractionnaire à afficher pour un rouleau.
float reelDisplayPos(const Game& g, uint8_t reel, uint32_t now);

void noteInput(Game& g, uint32_t now);

}  // namespace core
