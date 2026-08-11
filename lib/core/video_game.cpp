#include "video_game.h"

namespace core {

namespace {
// Chaque ligne gagnante reste visible ce temps-là avant de céder la place.
constexpr uint32_t kLineShowMs = 550;

Cue winCueFor(Tier t) {
    switch (t) {
        case Tier::Small: return Cue::WinSmall;
        case Tier::Mid: return Cue::WinMid;
        case Tier::Big: return Cue::WinBig;
        case Tier::Jackpot: return Cue::Jackpot;
        default: return Cue::None;
    }
}

Tier tierOfMultiplier(uint32_t mult, bool jackpot) {
    if (jackpot) return Tier::Jackpot;
    if (mult == 0) return Tier::None;
    if (mult >= 500) return Tier::Big;
    if (mult >= 100) return Tier::Mid;
    return Tier::Small;
}
}  // namespace

void pushVideoCue(VideoGame& g, Cue c) {
    const uint8_t next = static_cast<uint8_t>((g.cueTail + 1) % 6);
    if (next == g.cueHead) return;
    g.cueQueue[g.cueTail] = c;
    g.cueTail = next;
}

Cue takeVideoCue(VideoGame& g) {
    if (g.cueHead == g.cueTail) return Cue::None;
    const Cue c = g.cueQueue[g.cueHead];
    g.cueHead = static_cast<uint8_t>((g.cueHead + 1) % 6);
    return c;
}

VideoGame newVideoGame(uint32_t now, RngFn rng) {
    VideoGame g;
    g.reels = &videoReelSet();
    g.pay = &videoPaytable();
    g.lines = videoPaylines();
    g.econ = freshEconomy();
    g.phaseT0 = now;
    g.lastInputMs = now;
    for (uint8_t r = 0; r < g.reels->reels; ++r) {
        g.restPos[r] = static_cast<uint16_t>(drawBelow(rng, g.reels->len[r]));
        g.outcome.pos[r] = g.restPos[r];
    }
    fillGrid(*g.reels, g.outcome, kVideoRows);
    return g;
}

bool startVideoSpin(VideoGame& g, uint32_t now, RngFn rng, bool byPlayer) {
    if (g.phase == Phase::Spinning) return false;

    const uint32_t cost = videoStake(g.econ);
    if (byPlayer) {
        // Le tour coûte cinq mises : c'est ce total qu'il faut pouvoir payer.
        clampBetFor(g.econ, kVideoLines);
        if (videoStake(g.econ) > static_cast<uint32_t>(g.econ.credits)) return false;
        g.stake = static_cast<uint16_t>(videoStake(g.econ));
        g.econ.credits -= static_cast<int32_t>(g.stake);
    } else {
        g.stake = static_cast<uint16_t>(cost);  // démo : affiché, jamais débité
    }
    g.perLineStake = bet(g.econ);  // figée ici, relue au règlement

    spinGrid(*g.reels, rng, g.outcome, kVideoRows);
    evaluateGrid(*g.pay, g.lines, kVideoLines, g.reels->reels, g.outcome);

    for (uint8_t r = 0; r < g.reels->reels; ++r) {
        g.motion[r] = armReel(now, r, g.restPos[r], g.outcome.pos[r],
                              g.reels->len[r], static_cast<uint8_t>(3 + r));
    }
    g.phase = Phase::Spinning;
    g.phaseT0 = now;
    g.tier = Tier::None;
    g.reelsStopped = 0;
    g.shownWin = 0;
    g.payout = 0;
    g.bailedOut = false;
    g.attract = !byPlayer;
    if (byPlayer) {
        g.lastInputMs = now;
        ++g.spins;
        pushVideoCue(g, Cue::SpinStart);
    } else {
        g.lastAttractMs = now;
    }
    return true;
}

uint8_t updateVideoGame(VideoGame& g, uint32_t now, RngFn rng) {
    uint8_t justStopped = 0;

    switch (g.phase) {
        case Phase::Spinning: {
            uint8_t stopped = 0;
            for (uint8_t r = 0; r < g.reels->reels; ++r) {
                if (reelSettled(g.motion[r], now)) ++stopped;
            }
            if (stopped > g.reelsStopped) {
                justStopped = static_cast<uint8_t>(stopped - g.reelsStopped);
                if (!g.attract) {
                    for (uint8_t k = 0; k < justStopped; ++k) {
                        pushVideoCue(g, reelStopCue(static_cast<uint8_t>(g.reelsStopped + k)));
                    }
                }
                g.reelsStopped = stopped;
            }
            if (stopped == g.reels->reels) {
                for (uint8_t r = 0; r < g.reels->reels; ++r) {
                    g.restPos[r] = g.outcome.pos[r];
                }
                g.tier = tierOfMultiplier(g.outcome.totalMultiplier, g.outcome.jackpot);
                if (!g.attract) {
                    // La mise par ligne multiplie le gain, pas le total engagé.
                    g.payout = g.outcome.totalMultiplier * g.perLineStake;
                    award(g.econ, g.payout);
                    if (needsBailout(g.econ)) {
                        bailout(g.econ);
                        g.bailedOut = true;
                    }
                    clampBetFor(g.econ, kVideoLines);
                }
                if (g.bailedOut) {
                    g.phase = Phase::Bailout;
                    pushVideoCue(g, Cue::Bailout);
                } else if (g.tier == Tier::None) {
                    g.phase = Phase::Idle;
                } else {
                    g.phase = Phase::Celebrate;
                    if (!g.attract) pushVideoCue(g, winCueFor(g.tier));
                }
                g.phaseT0 = now;
            }
            break;
        }
        case Phase::Celebrate: {
            // Durée : le temps de montrer chaque ligne gagnante, au moins le
            // palier habituel.
            const uint32_t byLines = kLineShowMs * (g.outcome.winCount ? g.outcome.winCount : 1);
            const uint32_t dur = byLines > celebrateMs(g.tier) ? byLines : celebrateMs(g.tier);
            if (now - g.phaseT0 >= dur) {
                g.phase = Phase::Idle;
                g.phaseT0 = now;
            }
            break;
        }
        case Phase::Bailout:
            if (now - g.phaseT0 >= 2200) {
                g.phase = Phase::Idle;
                g.phaseT0 = now;
            }
            break;
        case Phase::Idle:
            if (now - g.lastInputMs >= kAttractDelayMs &&
                (g.lastAttractMs == 0 || now - g.lastAttractMs >= kAttractIntervalMs)) {
                startVideoSpin(g, now, rng, /*byPlayer=*/false);
            }
            break;
    }
    return justStopped;
}

float videoReelPos(const VideoGame& g, uint8_t reel, uint32_t now) {
    if (g.phase == Phase::Spinning) return reelPosition(g.motion[reel], now);
    return static_cast<float>(g.restPos[reel]);
}

void noteVideoInput(VideoGame& g, uint32_t now) {
    g.lastInputMs = now;
    if (g.phase == Phase::Idle) g.attract = false;
}

uint8_t highlightedLine(const VideoGame& g, uint32_t now) {
    if (g.phase != Phase::Celebrate || g.outcome.winCount == 0) return 0xFF;
    const uint32_t k = (now - g.phaseT0) / kLineShowMs;
    return g.outcome.wins[k % g.outcome.winCount].line;
}

}  // namespace core
