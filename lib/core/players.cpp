#include "players.h"

namespace core {

namespace {
bool sameName(const char* a, const char* b) {
    for (uint8_t i = 0; i <= kNameMax; ++i) {
        if (a[i] != b[i]) return false;
        if (a[i] == '\0') return true;
    }
    return true;
}
}  // namespace

bool nameValid(const char* name) {
    uint8_t n = 0;
    for (; name[n]; ++n) {
        if (n >= kNameMax) return false;
        const char c = name[n];
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
        if (!ok) return false;
    }
    return n > 0;
}

bool addOrSwitchPlayer(Roster& r, const char* name) {
    if (!nameValid(name)) return false;
    for (uint8_t i = 0; i < r.count; ++i) {
        if (sameName(r.players[i].name, name)) {
            r.current = i;
            return true;
        }
    }
    if (r.count >= kMaxPlayers) return false;
    Player& p = r.players[r.count];
    uint8_t i = 0;
    for (; name[i] && i < kNameMax; ++i) p.name[i] = name[i];
    p.name[i] = '\0';
    p.credits = kStartingCredits;
    p.spins = 0;
    p.bestWin = 0;
    for (uint8_t g = 0; g < kBetGames; ++g) p.bet[g] = kDefaultBetIndex;
    r.current = r.count;
    ++r.count;
    return true;
}

Player* currentPlayer(Roster& r) {
    if (r.count == 0 || r.current >= r.count) return nullptr;
    return &r.players[r.current];
}

void rankPlayers(const Roster& r, uint8_t* out) {
    for (uint8_t i = 0; i < r.count; ++i) out[i] = i;
    // Tri par insertion : 8 entrées au plus, la simplicité gagne.
    for (uint8_t i = 1; i < r.count; ++i) {
        const uint8_t v = out[i];
        int8_t j = static_cast<int8_t>(i) - 1;
        auto before = [&](uint8_t a, uint8_t b) {
            const Player &pa = r.players[a], &pb = r.players[b];
            if (pa.credits != pb.credits) return pa.credits > pb.credits;
            return pa.bestWin > pb.bestWin;
        };
        while (j >= 0 && before(v, out[j])) {
            out[j + 1] = out[j];
            --j;
        }
        out[j + 1] = v;
    }
}

void resetRoster(Roster& r) {
    r.count = 0;
    r.current = 0;
}

void syncPlayer(Roster& r, const Economy& e, uint32_t spins, uint32_t lastPayout) {
    Player* p = currentPlayer(r);
    if (!p) return;
    p->credits = e.credits;
    p->spins = spins;
    if (lastPayout > p->bestWin) p->bestWin = lastPayout;
}

}  // namespace core
