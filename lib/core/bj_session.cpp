#include "bj_session.h"

namespace core {

void pushBjCue(BjSession& s, Cue c) {
    const uint8_t next = static_cast<uint8_t>((s.cueTail + 1) % 6);
    if (next == s.cueHead) return;
    s.cueQueue[s.cueTail] = c;
    s.cueTail = next;
}

Cue takeBjCue(BjSession& s) {
    if (s.cueHead == s.cueTail) return Cue::None;
    const Cue c = s.cueQueue[s.cueHead];
    s.cueHead = static_cast<uint8_t>((s.cueHead + 1) % 6);
    return c;
}

BjSession newBjSession(uint32_t now) {
    BjSession s;
    // Ceinture et bretelles : les valeurs par défaut de Hand suffisent,
    // mais une session neuve doit être explicitement vide.
    handClear(s.bj.player);
    handClear(s.bj.dealer);
    s.bj.phase = BjPhase::Idle;
    s.bj.outcome = BjOutcome::None;
    s.bj.stake = 0;
    s.bj.payout = 0;
    s.econ = freshEconomy();
    s.phaseT0 = now;
    s.lastInputMs = now;
    return s;
}

bool bjStartHand(BjSession& s, uint32_t now, RngFn rng) {
    if (s.bj.phase == BjPhase::PlayerTurn || s.bj.phase == BjPhase::DealerTurn) {
        return false;
    }
    if (!bjDeal(s.bj, s.econ, rng)) return false;
    s.phaseT0 = now;
    s.lastStepMs = now;
    s.lastInputMs = now;
    s.revealed = 0;     // les quatre cartes apparaissent une à une
    s.choice = BjChoice::Hit;
    ++s.hands;
    pushBjCue(s, Cue::SpinStart);
    return true;
}

void bjMoveChoice(BjSession& s, int8_t delta, uint32_t now) {
    if (s.bj.phase != BjPhase::PlayerTurn) return;
    int8_t v = static_cast<int8_t>(static_cast<int8_t>(s.choice) + delta);
    if (v < 0) v = kBjChoices - 1;
    if (v >= static_cast<int8_t>(kBjChoices)) v = 0;
    // Doubler n'est pas toujours offert : on saute la case plutôt que de
    // proposer un choix qui ne ferait rien.
    if (static_cast<BjChoice>(v) == BjChoice::Double && !bjCanDouble(s.bj, s.econ)) {
        v = static_cast<int8_t>(delta > 0 ? 0 : kBjChoices - 2);
    }
    s.choice = static_cast<BjChoice>(v);
    s.lastInputMs = now;
    pushBjCue(s, Cue::BetChange);
}

void bjConfirm(BjSession& s, uint32_t now, RngFn rng) {
    s.lastInputMs = now;
    if (s.bj.phase != BjPhase::PlayerTurn) return;
    if (s.revealed < 4) return;  // pas d'action avant la fin de la donne

    switch (s.choice) {
        case BjChoice::Hit:
            bjAct(s.bj, BjAction::Hit, s.econ, rng);
            pushBjCue(s, Cue::ReelStop1);
            break;
        case BjChoice::Stand:
            bjAct(s.bj, BjAction::Stand, s.econ, rng);
            pushBjCue(s, Cue::ReelStop2);
            break;
        case BjChoice::Double:
            if (!bjCanDouble(s.bj, s.econ)) return;
            bjAct(s.bj, BjAction::Double, s.econ, rng);
            pushBjCue(s, Cue::ReelStop3);
            break;
    }
    s.phaseT0 = now;
    s.lastStepMs = now;
    if (!bjCanDouble(s.bj, s.econ) && s.choice == BjChoice::Double) {
        s.choice = BjChoice::Hit;
    }
}

namespace {
Cue outcomeCue(BjOutcome o) {
    switch (o) {
        case BjOutcome::PlayerBlackjack: return Cue::Jackpot;
        case BjOutcome::PlayerWin:
        case BjOutcome::DealerBust: return Cue::WinMid;
        case BjOutcome::Push: return Cue::BetChange;
        default: return Cue::None;
    }
}
}  // namespace

void bjUpdate(BjSession& s, uint32_t now, RngFn rng) {
    // Distribution progressive : quatre cartes, une toutes les 260 ms.
    if (s.revealed < 4) {
        if (now - s.lastStepMs >= kBjDealStepMs) {
            ++s.revealed;
            s.lastStepMs = now;
            pushBjCue(s, Cue::ReelStop1);
            if (s.revealed == 4 && s.bj.phase == BjPhase::Settle) {
                pushBjCue(s, outcomeCue(s.bj.outcome));  // blackjack immédiat
                s.phaseT0 = now;
            }
        }
        return;
    }

    if (s.bj.phase == BjPhase::DealerTurn) {
        if (now - s.lastStepMs >= kBjDealerStepMs) {
            s.lastStepMs = now;
            if (!bjDealerStep(s.bj, rng)) {
                bjSettle(s.bj, s.econ);
                pushBjCue(s, outcomeCue(s.bj.outcome));
                s.phaseT0 = now;
            } else {
                pushBjCue(s, Cue::ReelStop1);
            }
        }
    }
}

uint8_t bjVisiblePlayer(const BjSession& s) {
    if (s.revealed >= 4) return s.bj.player.n;
    // Ordre de la donne : joueur, croupier, joueur, croupier.
    return static_cast<uint8_t>((s.revealed + 1) / 2);
}

uint8_t bjVisibleDealer(const BjSession& s) {
    if (s.revealed >= 4) return s.bj.dealer.n;
    return static_cast<uint8_t>(s.revealed / 2);
}

bool bjHoleHidden(const BjSession& s) {
    return s.bj.phase == BjPhase::PlayerTurn;
}

BjAction bjBasicStrategy(const Hand& player, Card dealerUp, bool canDouble) {
    const HandValue v = handValue(player);
    // Valeur de la carte visible du croupier : l'As compte 11.
    const uint8_t up = dealerUp.rank == 1 ? 11
                     : (dealerUp.rank >= 10 ? 10 : dealerUp.rank);

    if (v.soft) {
        // Mains souples : on double plus volontiers, on ne reste jamais
        // sous 18 car l'As protège du saut.
        if (v.total >= 19) return BjAction::Stand;
        if (v.total == 18) {
            if (canDouble && up >= 3 && up <= 6) return BjAction::Double;
            return (up == 9 || up == 10 || up == 11) ? BjAction::Hit : BjAction::Stand;
        }
        if (canDouble && v.total >= 13 && up >= 4 && up <= 6) return BjAction::Double;
        return BjAction::Hit;
    }

    if (v.total >= 17) return BjAction::Stand;
    if (v.total >= 13) return up <= 6 ? BjAction::Stand : BjAction::Hit;
    if (v.total == 12) return (up >= 4 && up <= 6) ? BjAction::Stand : BjAction::Hit;
    if (v.total == 11) return canDouble ? BjAction::Double : BjAction::Hit;
    if (v.total == 10) return (canDouble && up <= 9) ? BjAction::Double : BjAction::Hit;
    if (v.total == 9) return (canDouble && up >= 3 && up <= 6) ? BjAction::Double
                                                              : BjAction::Hit;
    return BjAction::Hit;
}

}  // namespace core
