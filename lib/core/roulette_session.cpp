#include "roulette_session.h"

namespace core {

void pushRltCue(RouletteSession& s, Cue c) {
    if (s.attract) return;
    const uint8_t next = static_cast<uint8_t>((s.cueTail + 1) % 6);
    if (next == s.cueHead) return;
    s.cueQueue[s.cueTail] = c;
    s.cueTail = next;
}

Cue takeRltCue(RouletteSession& s) {
    if (s.cueHead == s.cueTail) return Cue::None;
    const Cue c = s.cueQueue[s.cueHead];
    s.cueHead = static_cast<uint8_t>((s.cueHead + 1) % 6);
    return c;
}

RouletteSession newRouletteSession(uint32_t now, RngFn rng) {
    RouletteSession s;
    s.econ = freshEconomy();
    s.phaseT0 = now;
    s.restIndex = static_cast<uint8_t>(drawBelow(rng, kPockets));
    s.winNumber = pocketAt(s.restIndex);
    return s;
}

void rltCycleBet(RouletteSession& s, int8_t delta) {
    if (s.phase == RltPhase::Spinning) return;
    int8_t v = static_cast<int8_t>(static_cast<int8_t>(s.kind) + delta);
    if (v < 0) v = kBetKinds - 1;
    if (v >= static_cast<int8_t>(kBetKinds)) v = 0;
    s.kind = static_cast<BetKind>(v);
    pushRltCue(s, Cue::BetChange);
}

void rltCycleNumber(RouletteSession& s, int8_t delta) {
    if (s.phase == RltPhase::Spinning) return;
    if (s.kind != BetKind::Straight) return;
    int16_t v = static_cast<int16_t>(s.straight + delta);
    if (v < 0) v = kPockets - 1;
    if (v >= static_cast<int16_t>(kPockets)) v = 0;
    s.straight = static_cast<uint8_t>(v);
    pushRltCue(s, Cue::BetChange);
}

bool rltSpin(RouletteSession& s, uint32_t now, RngFn rng, bool byPlayer) {
    if (s.phase == RltPhase::Spinning) return false;
    clampBet(s.econ);
    if (!canSpin(s.econ)) return false;

    s.attract = !byPlayer;
    s.stake = bet(s.econ);
    if (!s.attract) placeBet(s.econ);

    const uint8_t idx = static_cast<uint8_t>(drawBelow(rng, kPockets));
    s.winNumber = pocketAt(idx);
    // Cinq tours au moins : la bille doit avoir vraiment tourné avant de
    // se poser, sinon le résultat semble décidé d'avance.
    s.motion = armReelMs(now, s.restIndex, idx, kPockets, 5, kRltSpinMs);
    s.restIndex = idx;
    s.lastTickPocket = -1;
    s.lastTickMs = now;

    s.phase = RltPhase::Spinning;
    s.phaseT0 = now;
    s.won = false;
    s.payout = 0;
    s.bailedOut = false;
    ++s.spins;
    pushRltCue(s, Cue::SpinStart);
    return true;
}

void rltUpdate(RouletteSession& s, uint32_t now) {
    switch (s.phase) {
        case RltPhase::Spinning: {
            // Cliquetis : une fois par case franchie, mais jamais plus
            // souvent que toutes les 70 ms — au lancement la bille passe
            // une case toutes les 8 ms, on n'entendrait qu'un buzz.
            const int32_t pocket = static_cast<int32_t>(reelPosition(s.motion, now));
            if (pocket != s.lastTickPocket && now - s.lastTickMs >= 70) {
                s.lastTickPocket = pocket;
                s.lastTickMs = now;
                pushRltCue(s, Cue::Tick);
            }
            if (!reelSettled(s.motion, now)) break;
            s.won = betWins(s.kind, s.straight, s.winNumber);
            if (s.attract) {
                s.payout = s.won ? roulettePayout(s.kind) * s.stake : 0;
                s.phase = RltPhase::Result;
                s.phaseT0 = now;
                break;
            }
            if (s.won) {
                s.payout = static_cast<uint32_t>(roulettePayout(s.kind)) * s.stake;
                award(s.econ, s.payout);
                pushRltCue(s, s.kind == BetKind::Straight ? Cue::WinBig : Cue::WinMid);
            } else {
                pushRltCue(s, Cue::ReelStop3);
            }
            if (needsBailout(s.econ)) {
                bailout(s.econ);
                s.bailedOut = true;
                pushRltCue(s, Cue::Bailout);
            }
            clampBet(s.econ);
            s.phase = RltPhase::Result;
            s.phaseT0 = now;
            break;
        }
        case RltPhase::Result:
            if (now - s.phaseT0 >= kRltResultMs) {
                s.phase = RltPhase::Idle;
                s.phaseT0 = now;
            }
            break;
        case RltPhase::Idle:
            break;
    }
}

float rltWheelPos(const RouletteSession& s, uint32_t now) {
    if (s.phase == RltPhase::Spinning) return reelPosition(s.motion, now);
    return static_cast<float>(s.restIndex);
}

}  // namespace core
