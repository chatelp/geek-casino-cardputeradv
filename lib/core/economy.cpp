#include "economy.h"

namespace core {

Economy freshEconomy() { return Economy{kStartingCredits, kDefaultBetIndex}; }

void raiseBet(Economy& e) {
    if (e.betIndex + 1 < kBetSteps &&
        kBetLadder[e.betIndex + 1] <= e.credits) {
        ++e.betIndex;
    }
}

void lowerBet(Economy& e) {
    if (e.betIndex > 0) --e.betIndex;
}

void clampBet(Economy& e) {
    while (e.betIndex > 0 && kBetLadder[e.betIndex] > e.credits) --e.betIndex;
}

bool canSpin(const Economy& e) { return e.credits >= kBetLadder[e.betIndex]; }

void placeBet(Economy& e) {
    if (canSpin(e)) e.credits -= kBetLadder[e.betIndex];
}

void award(Economy& e, uint32_t amount) {
    e.credits += static_cast<int32_t>(amount);
}

bool needsBailout(const Economy& e) { return e.credits < kBetLadder[0]; }

void bailout(Economy& e) {
    if (needsBailout(e)) {
        e.credits = kBailoutCredits;
        clampBet(e);
    }
}

}  // namespace core
