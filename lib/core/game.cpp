#include "game.h"

namespace core {

Tier tierOf(const WinResult& w) {
    if (w.jackpot) return Tier::Jackpot;
    if (w.multiplier == 0) return Tier::None;
    if (w.multiplier >= 50) return Tier::Big;
    if (w.multiplier >= 10) return Tier::Mid;
    return Tier::Small;
}

uint32_t celebrateMs(Tier t) {
    switch (t) {
        case Tier::None: return 0;
        case Tier::Small: return 400;
        case Tier::Mid: return 900;
        case Tier::Big: return 1600;
        case Tier::Jackpot: return 3000;
    }
    return 0;
}

Game newGame(uint32_t now, RngFn rng) {
    Game g;
    g.machine = mvpMachine();
    g.phaseT0 = now;
    g.lastInputMs = now;
    // Positions de repos tirées au sort : la machine n'affiche pas la même
    // chose à chaque allumage.
    for (uint8_t r = 0; r < g.machine.reels->reels; ++r) {
        g.restPos[r] = static_cast<uint16_t>(drawBelow(rng, g.machine.reels->len[r]));
    }
    return g;
}

bool startSpin(Game& g, uint32_t now, RngFn rng, bool byPlayer) {
    if (g.phase == Phase::Spinning) return false;
    if (!playSpin(g.machine, rng, g.outcome)) return false;

    const ReelSet& rs = *g.machine.reels;
    for (uint8_t r = 0; r < rs.reels; ++r) {
        // Le dernier rouleau tourne plus longtemps ET fait plus de tours :
        // sans les deux, la cascade se voit mais ne se sent pas.
        g.motion[r] = armReel(now, r, g.restPos[r], g.outcome.pos[r],
                              rs.len[r], static_cast<uint8_t>(3 + r));
    }
    g.phase = Phase::Spinning;
    g.phaseT0 = now;
    g.tier = Tier::None;
    g.reelsStopped = 0;
    g.attract = !byPlayer;
    if (byPlayer) g.lastInputMs = now;
    return true;
}

uint8_t updateGame(Game& g, uint32_t now, RngFn rng) {
    uint8_t justStopped = 0;
    const ReelSet& rs = *g.machine.reels;

    switch (g.phase) {
        case Phase::Spinning: {
            uint8_t stopped = 0;
            for (uint8_t r = 0; r < rs.reels; ++r) {
                if (reelSettled(g.motion[r], now)) ++stopped;
            }
            if (stopped > g.reelsStopped) {
                justStopped = static_cast<uint8_t>(stopped - g.reelsStopped);
                g.reelsStopped = stopped;
            }
            if (stopped == rs.reels) {
                for (uint8_t r = 0; r < rs.reels; ++r) g.restPos[r] = g.outcome.pos[r];
                g.tier = tierOf(g.outcome.win);
                if (g.outcome.bailedOut) {
                    g.phase = Phase::Bailout;
                } else {
                    g.phase = g.tier == Tier::None ? Phase::Idle : Phase::Celebrate;
                }
                g.phaseT0 = now;
            }
            break;
        }
        case Phase::Celebrate:
            if (now - g.phaseT0 >= celebrateMs(g.tier)) {
                g.phase = Phase::Idle;
                g.phaseT0 = now;
            }
            break;
        case Phase::Bailout:
            // Assez long pour être lu : c'est le seul moment où la machine
            // s'adresse vraiment au joueur.
            if (now - g.phaseT0 >= 2200) {
                g.phase = Phase::Idle;
                g.phaseT0 = now;
            }
            break;
        case Phase::Idle:
            if (now - g.lastInputMs >= kAttractDelayMs) {
                startSpin(g, now, rng, /*byPlayer=*/false);
            }
            break;
    }
    return justStopped;
}

float reelDisplayPos(const Game& g, uint8_t reel, uint32_t now) {
    if (g.phase == Phase::Spinning) return reelPosition(g.motion[reel], now);
    return static_cast<float>(g.restPos[reel]);
}

void noteInput(Game& g, uint32_t now) { g.lastInputMs = now; }

}  // namespace core
