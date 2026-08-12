#include "vp_session.h"

namespace core {

void pushVpCue(VpSession& s, Cue c) {
    if (s.attract) return;
    const uint8_t next = static_cast<uint8_t>((s.cueTail + 1) % 6);
    if (next == s.cueHead) return;
    s.cueQueue[s.cueTail] = c;
    s.cueTail = next;
}

Cue takeVpCue(VpSession& s) {
    if (s.cueHead == s.cueTail) return Cue::None;
    const Cue c = s.cueQueue[s.cueHead];
    s.cueHead = static_cast<uint8_t>((s.cueHead + 1) % 6);
    return c;
}

bool vpIsMaxBet(const Economy& e) { return e.betIndex + 1 == kBetSteps; }

VpSession newVpSession(uint32_t now) {
    VpSession s;
    handClear(s.hand);
    s.econ = freshEconomy();
    s.phaseT0 = now;
    return s;
}

bool vpDeal(VpSession& s, uint32_t now, RngFn rng, bool byPlayer) {
    if (s.phase == VpPhase::Holding) return false;
    clampBet(s.econ);
    if (!canSpin(s.econ)) return false;

    s.attract = !byPlayer;
    s.stake = bet(s.econ);
    s.maxBet = vpIsMaxBet(s.econ);
    if (!s.attract) placeBet(s.econ);

    // Jeu neuf à chaque main : c'est ce dont dépendent les probabilités.
    shuffleDeck(s.deck, rng);
    handClear(s.hand);
    for (uint8_t i = 0; i < kPokerHandSize; ++i) handAdd(s.hand, dealFromDeck(s.deck));
    for (uint8_t i = 0; i < kPokerHandSize; ++i) s.held[i] = false;

    s.cursor = 0;
    s.revealed = 0;
    s.replacing = 0;
    s.result = PokerRank::None;
    s.payout = 0;
    s.bailedOut = false;
    s.phase = VpPhase::Holding;
    s.phaseT0 = now;
    s.lastStepMs = now;
    ++s.hands;
    pushVpCue(s, Cue::SpinStart);
    return true;
}

void vpMoveCursor(VpSession& s, int8_t delta) {
    if (s.phase != VpPhase::Holding) return;
    int8_t v = static_cast<int8_t>(s.cursor + delta);
    if (v < 0) v = kVpSlots - 1;
    if (v >= static_cast<int8_t>(kVpSlots)) v = 0;
    s.cursor = static_cast<uint8_t>(v);
}

namespace {

Cue cueForRank(PokerRank r) {
    if (r == PokerRank::RoyalFlush || r == PokerRank::StraightFlush) return Cue::Jackpot;
    if (r >= PokerRank::FullHouse) return Cue::WinBig;
    if (r >= PokerRank::Straight) return Cue::WinMid;
    if (r != PokerRank::None) return Cue::WinSmall;
    return Cue::None;
}

void settle(VpSession& s, uint32_t now) {
    s.result = rankHand(s.hand);
    s.payout = static_cast<uint32_t>(pokerPayout(s.result, s.maxBet)) * s.stake;
    if (s.attract) { s.phase = VpPhase::Result; s.phaseT0 = now; return; }
    award(s.econ, s.payout);
    if (needsBailout(s.econ)) {
        bailout(s.econ);
        s.bailedOut = true;
    }
    clampBet(s.econ);
    s.phase = VpPhase::Result;
    s.phaseT0 = now;
    pushVpCue(s, cueForRank(s.result));
}

}  // namespace

void vpConfirm(VpSession& s, uint32_t now, RngFn rng) {
    if (s.phase == VpPhase::Result || s.phase == VpPhase::Idle) return;
    if (s.revealed < kPokerHandSize) return;  // pas d'action pendant la donne

    if (s.cursor == kVpDrawSlot) {
        // Échange : les cartes non gardées sont remplacées par les
        // suivantes du MÊME jeu — jamais d'un jeu neuf, sinon une carte
        // déjà en main pourrait revenir.
        for (uint8_t i = 0; i < kPokerHandSize; ++i) {
            if (!s.held[i]) s.hand.c[i] = dealFromDeck(s.deck);
        }
        s.replacing = kPokerHandSize;
        settle(s, now);
        return;
    }
    s.held[s.cursor] = !s.held[s.cursor];
    pushVpCue(s, Cue::BetChange);
}

void vpUpdate(VpSession& s, uint32_t now, RngFn rng) {
    (void)rng;
    if (s.phase == VpPhase::Holding && s.revealed < kPokerHandSize) {
        if (now - s.lastStepMs >= kVpDealStepMs) {
            ++s.revealed;
            s.lastStepMs = now;
            pushVpCue(s, Cue::ReelStop1);
        }
    }
}

uint8_t vpVisible(const VpSession& s) {
    if (s.phase == VpPhase::Idle) return 0;
    return s.revealed;
}

}  // namespace core
