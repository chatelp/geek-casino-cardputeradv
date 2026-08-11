// Machine à états du format vidéo 5×3 — logique pure, temps injecté.
// Même discipline que game.h : le rythme se teste sans écran.
#pragma once
#include <cstdint>

#include "economy.h"
#include "game.h"       // Phase, Tier, Cue, reel_motion
#include "multiline.h"
#include "rng.h"

namespace core {

struct VideoGame {
    const ReelSet* reels = nullptr;
    const Paytable* pay = nullptr;
    const Payline* lines = nullptr;
    Economy econ;

    GridOutcome outcome;
    ReelMotion motion[kMaxReels];
    uint16_t restPos[kMaxReels];

    Phase phase = Phase::Idle;
    Tier tier = Tier::None;
    uint32_t phaseT0 = 0;
    uint32_t lastInputMs = 0;
    uint32_t lastAttractMs = 0;
    bool attract = false;
    uint8_t reelsStopped = 0;
    uint32_t spins = 0;

    uint16_t stake = 0;        // total engagé sur le tour (mise × lignes)
    // Mise PAR LIGNE au moment du lancement. Le gain se calcule sur elle,
    // jamais sur la mise affichée à l'arrivée : sinon monter la mise
    // pendant la rotation paierait plus que ce qu'on a engagé.
    uint16_t perLineStake = 0;
    uint32_t payout = 0;
    bool bailedOut = false;
    // Pendant la célébration, les lignes gagnantes défilent une à une :
    // cinq lignes allumées ensemble ne se lisent pas.
    uint8_t shownWin = 0;

    Cue cueQueue[6] = {};
    uint8_t cueHead = 0, cueTail = 0;
};

// La mise affichée est engagée SUR CHAQUE ligne : le tour coûte
// mise × kVideoLines. C'est la convention des machines vidéo.
inline uint32_t videoStake(const Economy& e) {
    return static_cast<uint32_t>(bet(e)) * kVideoLines;
}

VideoGame newVideoGame(uint32_t now, RngFn rng);
bool startVideoSpin(VideoGame& g, uint32_t now, RngFn rng, bool byPlayer = true);
uint8_t updateVideoGame(VideoGame& g, uint32_t now, RngFn rng);
float videoReelPos(const VideoGame& g, uint8_t reel, uint32_t now);
void noteVideoInput(VideoGame& g, uint32_t now);

void pushVideoCue(VideoGame& g, Cue c);
Cue takeVideoCue(VideoGame& g);

// Ligne mise en avant à l'instant `now`, ou 0xFF si aucune.
uint8_t highlightedLine(const VideoGame& g, uint32_t now);

}  // namespace core
