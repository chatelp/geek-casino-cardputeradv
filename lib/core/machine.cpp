#include "machine.h"

namespace core {

Machine mvpMachine() {
    return Machine{&mvpReelSet(), &mvpPaytable(), freshEconomy()};
}

bool playSpin(Machine& m, RngFn rng, SpinOutcome& out, bool charge) {
    out = SpinOutcome{};

    if (charge) {
        // La mise suit le solde vers le bas plutôt que de bloquer le joueur.
        clampBet(m.econ);
        if (!canSpin(m.econ)) return false;
    }

    out.stake = bet(m.econ);
    if (charge) placeBet(m.econ);

    spin(*m.reels, rng, out.pos, out.sym);
    out.win = evaluateLine(*m.pay, out.sym, m.reels->reels);
    out.payout = static_cast<uint32_t>(out.win.multiplier) * out.stake;

    return true;
}

void settleSpin(Machine& m, SpinOutcome& out, bool charge) {
    if (!charge) return;   // la démo ne touche à rien
    award(m.econ, out.payout);
    // Garde-fou : personne ne repart ruiné.
    if (needsBailout(m.econ)) {
        bailout(m.econ);
        out.bailedOut = true;
    }
    clampBet(m.econ);
}

}  // namespace core
