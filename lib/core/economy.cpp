#include "economy.h"

namespace core {

Economy freshEconomy() { return Economy{kStartingCredits, kDefaultBetIndex}; }

void raiseBetFor(Economy& e, uint8_t linesPerSpin) {
    if (linesPerSpin == 0) linesPerSpin = 1;
    if (e.betIndex + 1 < kBetSteps &&
        static_cast<int32_t>(kBetLadder[e.betIndex + 1]) * linesPerSpin <= e.credits) {
        ++e.betIndex;
    }
}

void lowerBet(Economy& e) {
    if (e.betIndex > 0) --e.betIndex;
}

void clampBetFor(Economy& e, uint8_t linesPerSpin) {
    if (linesPerSpin == 0) linesPerSpin = 1;
    while (e.betIndex > 0 &&
           static_cast<int32_t>(kBetLadder[e.betIndex]) * linesPerSpin > e.credits) {
        --e.betIndex;
    }
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
