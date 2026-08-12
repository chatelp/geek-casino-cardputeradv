#include "game.h"

namespace core {

Tier tierOf(const LineWin& w) {
    if (w.jackpot) return Tier::Jackpot;
    if (w.multiplier == 0) return Tier::None;
    if (w.multiplier >= 50) return Tier::Big;
    if (w.multiplier >= 10) return Tier::Mid;
    return Tier::Small;
}

uint32_t celebrateMs(Tier t) {
    // Durées revues après essai en main : 400 ms pour un petit gain se
    // lisait comme un clignotement, pas comme une récompense. L'escalade
    // se joue maintenant sur l'INTENSITÉ autant que sur la durée.
    switch (t) {
        case Tier::None: return 0;
        case Tier::Small: return 1200;
        case Tier::Mid: return 1800;
        case Tier::Big: return 2600;
        case Tier::Jackpot: return 4000;
    }
    return 0;
}

float celebrateProgress(Phase phase, Tier tier, uint32_t phaseT0, uint32_t now) {
    if (phase != Phase::Celebrate) return 1.0f;
    const uint32_t dur = celebrateMs(tier);
    if (dur == 0 || now <= phaseT0) return 0.0f;
    const uint32_t dt = now - phaseT0;
    if (dt >= dur) return 1.0f;
    return static_cast<float>(dt) / static_cast<float>(dur);
}

uint32_t countedPayout(uint32_t payout, float progress) {
    if (progress >= kCountFraction) return payout;  // exact, jamais approché
    const float k = progress / kCountFraction;
    // Décélération : le compteur ralentit en approchant, comme un
    // mécanisme qui se cale.
    const float e = 1.0f - (1.0f - k) * (1.0f - k);
    return static_cast<uint32_t>(static_cast<float>(payout) * e);
}

void pushCue(Game& g, Cue c) {
    const uint8_t next = static_cast<uint8_t>((g.cueTail + 1) % 6);
    if (next == g.cueHead) return;  // pleine : on préfère perdre le nouveau
    g.cueQueue[g.cueTail] = c;      // son que décaler tous les suivants
    g.cueTail = next;
}

Cue takeCue(Game& g) {
    if (g.cueHead == g.cueTail) return Cue::None;
    const Cue c = g.cueQueue[g.cueHead];
    g.cueHead = static_cast<uint8_t>((g.cueHead + 1) % 6);
    return c;
}

namespace {
Cue winCue(Tier t) {
    switch (t) {
        case Tier::Small: return Cue::WinSmall;
        case Tier::Mid: return Cue::WinMid;
        case Tier::Big: return Cue::WinBig;
        case Tier::Jackpot: return Cue::Jackpot;
        default: return Cue::None;
    }
}
}  // namespace

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
    // Le mode démo joue gratuitement : tirage réel, jetons intouchés.
    if (!playSpin(g.machine, rng, g.outcome, /*charge=*/byPlayer)) return false;

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
    else g.lastAttractMs = now;
    // La démo est muette : elle attire l'œil, elle n'impose rien à la pièce.
    if (byPlayer) pushCue(g, Cue::SpinStart);
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
                if (!g.attract) {
                    for (uint8_t k = 0; k < justStopped; ++k) {
                        pushCue(g, reelStopCue(static_cast<uint8_t>(g.reelsStopped + k)));
                    }
                }
                g.reelsStopped = stopped;
            }
            if (stopped == rs.reels) {
                for (uint8_t r = 0; r < rs.reels; ++r) g.restPos[r] = g.outcome.pos[r];
                g.tier = tierOf(g.outcome.win);
                if (g.outcome.bailedOut) {
                    g.phase = Phase::Bailout;
                    pushCue(g, Cue::Bailout);  // impossible en démo (gratuite)
                } else {
                    g.phase = g.tier == Tier::None ? Phase::Idle : Phase::Celebrate;
                    if (g.tier != Tier::None && !g.attract) pushCue(g, winCue(g.tier));
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
            // Deux conditions : le joueur est parti (c'est l'app qui le
            // décide, via lastInputMs) ET la démo a laissé passer un
            // moment depuis son tour précédent.
            if (g.demoArmed &&
                (g.lastAttractMs == 0 || now - g.lastAttractMs >= kAttractIntervalMs)) {
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

void noteInput(Game& g, uint32_t now) {
    g.lastInputMs = now;
    // Le premier geste du joueur reprend la main : la démo cesse d'être
    // affichée (et le tour de démo en cours finit ses rouleaux en gris).
    if (g.phase == Phase::Idle) g.attract = false;
}

}  // namespace core
