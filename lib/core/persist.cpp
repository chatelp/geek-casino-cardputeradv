#include "persist.h"

#include <cstddef>

namespace core {

uint32_t saveChecksum(const SaveData& s) {
    // FNV-1a sur tout sauf `sum` lui-même.
    uint32_t h = 2166136261u;
    const uint8_t* p = reinterpret_cast<const uint8_t*>(&s);
    const size_t n = offsetof(SaveData, sum);
    for (size_t i = 0; i < n; ++i) {
        h ^= p[i];
        h *= 16777619u;
    }
    return h;
}

SaveData makeSave(const Roster& r, const Settings& st) {
    SaveData s{};
    s.magic = kSaveMagic;
    s.version = kSaveVersion;
    s.playerCount = r.count;
    s.currentPlayer = r.current;
    s.settings = st;
    for (uint8_t i = 0; i < r.count && i < kMaxPlayers; ++i) s.players[i] = r.players[i];
    s.sum = saveChecksum(s);
    return s;
}

bool saveValid(const SaveData& s) {
    if (s.magic != kSaveMagic || s.version != kSaveVersion) return false;
    if (s.playerCount > kMaxPlayers) return false;
    if (s.playerCount > 0 && s.currentPlayer >= s.playerCount) return false;
    if (s.settings.volume > 3 || s.settings.muted > 1 || s.settings.slotSkin > 1) return false;
    for (uint8_t i = 0; i < s.playerCount; ++i) {
        if (!nameValid(s.players[i].name)) return false;
        if (s.players[i].credits < 0) return false;
    }
    return s.sum == saveChecksum(s);
}

bool applySave(const SaveData& s, Roster& r, Settings& st) {
    if (!saveValid(s)) return false;
    r.count = s.playerCount;
    r.current = s.currentPlayer;
    for (uint8_t i = 0; i < s.playerCount; ++i) r.players[i] = s.players[i];
    st = s.settings;
    return true;
}

}  // namespace core
