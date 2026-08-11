#include "blackjack.h"

namespace core {

uint32_t bjPayoutFor(BjOutcome o, uint16_t stake) {
    switch (o) {
        case BjOutcome::PlayerBlackjack:
            // 3:2 — l'arrondi va au joueur, pas à la maison. Sur un jouet,
            // c'est le seul arrondi défendable.
            return stake + (static_cast<uint32_t>(stake) * 3 + 1) / 2;
        case BjOutcome::PlayerWin:
        case BjOutcome::DealerBust:
            return static_cast<uint32_t>(stake) * 2;
        case BjOutcome::Push:
            return stake;  // mise rendue
        default:
            return 0;
    }
}

bool bjDeal(Blackjack& b, Economy& e, RngFn rng) {
    clampBet(e);
    if (!canSpin(e)) return false;

    if (needsShuffle(b.shoe)) shuffleShoe(b.shoe, rng);

    b.stake = bet(e);
    placeBet(e);
    b.doubled = false;
    b.bailedOut = false;
    b.outcome = BjOutcome::None;
    b.payout = 0;
    handClear(b.player);
    handClear(b.dealer);

    // Ordre réel : joueur, croupier, joueur, croupier (la seconde du
    // croupier reste face cachée jusqu'à son tour).
    handAdd(b.player, dealCard(b.shoe, rng));
    handAdd(b.dealer, dealCard(b.shoe, rng));
    handAdd(b.player, dealCard(b.shoe, rng));
    handAdd(b.dealer, dealCard(b.shoe, rng));
    ++b.hands;

    // Blackjack immédiat : le coup est joué avant même de commencer.
    if (isBlackjack(b.player) || isBlackjack(b.dealer)) {
        b.phase = BjPhase::Settle;
        bjSettle(b, e);
    } else {
        b.phase = BjPhase::PlayerTurn;
    }
    return true;
}

bool bjCanDouble(const Blackjack& b, const Economy& e) {
    return b.phase == BjPhase::PlayerTurn && b.player.n == 2 && !b.doubled &&
           e.credits >= static_cast<int32_t>(b.stake);
}

void bjAct(Blackjack& b, BjAction a, Economy& e, RngFn rng) {
    if (b.phase != BjPhase::PlayerTurn) return;

    switch (a) {
        case BjAction::Hit:
            handAdd(b.player, dealCard(b.shoe, rng));
            if (isBust(b.player)) {
                b.outcome = BjOutcome::PlayerBust;
                b.phase = BjPhase::Settle;
                bjSettle(b, e);
            }
            break;

        case BjAction::Double:
            if (!bjCanDouble(b, e)) break;
            // Seconde mise, puis une carte et une seule.
            e.credits -= static_cast<int32_t>(b.stake);
            b.stake = static_cast<uint16_t>(b.stake * 2);
            b.doubled = true;
            handAdd(b.player, dealCard(b.shoe, rng));
            if (isBust(b.player)) {
                b.outcome = BjOutcome::PlayerBust;
                b.phase = BjPhase::Settle;
                bjSettle(b, e);
            } else {
                b.phase = BjPhase::DealerTurn;
            }
            break;

        case BjAction::Stand:
            b.phase = BjPhase::DealerTurn;
            break;
    }
}

bool bjDealerStep(Blackjack& b, RngFn rng) {
    if (b.phase != BjPhase::DealerTurn) return false;

    const HandValue v = handValue(b.dealer);
    // S17 : le croupier reste sur 17, y compris souple.
    if (v.total < kDealerStandsOn) {
        handAdd(b.dealer, dealCard(b.shoe, rng));
        return true;
    }
    b.phase = BjPhase::Settle;
    return false;
}

void bjSettle(Blackjack& b, Economy& e) {
    if (b.outcome == BjOutcome::None) {
        const bool pbj = isBlackjack(b.player);
        const bool dbj = isBlackjack(b.dealer);
        const HandValue pv = handValue(b.player);
        const HandValue dv = handValue(b.dealer);

        if (pbj && dbj) {
            b.outcome = BjOutcome::Push;
        } else if (pbj) {
            b.outcome = BjOutcome::PlayerBlackjack;
        } else if (dbj) {
            b.outcome = BjOutcome::DealerWin;
        } else if (pv.total > 21) {
            b.outcome = BjOutcome::PlayerBust;
        } else if (dv.total > 21) {
            b.outcome = BjOutcome::DealerBust;
        } else if (pv.total > dv.total) {
            b.outcome = BjOutcome::PlayerWin;
        } else if (pv.total < dv.total) {
            b.outcome = BjOutcome::DealerWin;
        } else {
            b.outcome = BjOutcome::Push;
        }
    }

    b.payout = bjPayoutFor(b.outcome, b.stake);
    award(e, b.payout);
    b.phase = BjPhase::Settle;

    // Même garde-fou que la machine à sous : personne ne repart ruiné.
    if (needsBailout(e)) {
        bailout(e);
        b.bailedOut = true;
    }
    clampBet(e);
}

}  // namespace core
