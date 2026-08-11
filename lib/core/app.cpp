#include "app.h"

namespace core {

App newApp(uint32_t now, RngFn rng) {
    App a;
    a.game = newGame(now, rng);
    return a;
}

void enterFromSave(App& a) {
    Player* p = currentPlayer(a.roster);
    if (!p) {
        a.screen = AppScreen::NameEntry;
        return;
    }
    a.game.machine.econ.credits = p->credits;
    a.game.spins = p->spins;
    clampBet(a.game.machine.econ);
    a.screen = AppScreen::Lobby;
}

namespace {

void syncAndMarkDirty(App& a) {
    syncPlayer(a.roster, a.game.machine.econ, a.game.spins,
               a.game.outcome.payout);
    a.dirty = true;
}

void commitName(App& a) {
    if (a.nameEntry.len == 0) return;
    if (!addOrSwitchPlayer(a.roster, a.nameEntry.buf)) {
        a.nameEntry.rosterFull = true;  // plein et nom inconnu
        return;
    }
    // Nouveau joueur ou bascule : l'économie suit l'entrée courante.
    Player* p = currentPlayer(a.roster);
    a.game.machine.econ.credits = p->credits;
    a.game.machine.econ.betIndex = kDefaultBetIndex;
    a.game.spins = p->spins;
    clampBet(a.game.machine.econ);
    a.nameEntry = NameEntry{};
    a.dirty = true;
    a.screen = AppScreen::Lobby;
}

void keyLobby(App& a, AppKey k, uint32_t now, RngFn rng) {
    switch (k) {
        case AppKey::Up:
            if (a.lobbyIndex > 0) { --a.lobbyIndex; pushCue(a.game, Cue::BetChange); }
            break;
        case AppKey::Down:
            if (a.lobbyIndex < 2) { ++a.lobbyIndex; pushCue(a.game, Cue::BetChange); }
            break;
        case AppKey::Confirm:
            if (a.lobbyIndex == 0) {  // seuls SLOTS existe
                noteInput(a.game, now);
                a.screen = AppScreen::Slot;
            }
            break;
        case AppKey::Settings:
            a.menuIndex = 0;
            a.resetArmed = false;
            a.screen = AppScreen::GlobalSettings;
            break;
        case AppKey::Board:
            a.screen = AppScreen::Leaderboard;
            break;
        case AppKey::Help:
            if (a.lobbyIndex == 0) {
                a.helpReturn = AppScreen::Lobby;
                a.screen = AppScreen::SlotHelp;
            }
            break;
        case AppKey::Back:
            a.quitRequested = true;  // ignoré par l'appareil
            break;
        default:
            break;
    }
}

void keySlot(App& a, AppKey k, uint32_t now, RngFn rng) {
    switch (k) {
        case AppKey::Confirm:
            noteInput(a.game, now);
            if (startSpin(a.game, now, rng)) {
                ++a.game.spins;
                syncAndMarkDirty(a);
            }
            break;
        case AppKey::Left:
            noteInput(a.game, now);
            lowerBet(a.game.machine.econ);
            pushCue(a.game, Cue::BetChange);
            break;
        case AppKey::Right:
            noteInput(a.game, now);
            raiseBet(a.game.machine.econ);
            pushCue(a.game, Cue::BetChange);
            break;
        case AppKey::Help:
            a.helpReturn = AppScreen::Slot;
            a.screen = AppScreen::SlotHelp;
            break;
        case AppKey::Settings:
            a.menuIndex = 0;
            a.screen = AppScreen::SlotSettings;
            break;
        case AppKey::Back:
            syncAndMarkDirty(a);
            a.screen = AppScreen::Lobby;
            break;
        default:
            noteInput(a.game, now);
            break;
    }
}

void keyGlobalSettings(App& a, AppKey k) {
    // Lignes : 0 SOUND, 1 VOLUME, 2 PLAYER, 3 RESET LEADERBOARD
    switch (k) {
        case AppKey::Up:
            if (a.menuIndex > 0) --a.menuIndex;
            a.resetArmed = false;
            break;
        case AppKey::Down:
            if (a.menuIndex + 1 < kGlobalSettingsRows) ++a.menuIndex;
            a.resetArmed = false;
            break;
        case AppKey::Left:
        case AppKey::Right: {
            const bool inc = k == AppKey::Right;
            if (a.menuIndex == 0) {
                a.settings.muted = a.settings.muted ? 0 : 1;
                a.dirty = true;
            } else if (a.menuIndex == 1) {
                if (inc && a.settings.volume < 3) ++a.settings.volume;
                if (!inc && a.settings.volume > 0) --a.settings.volume;
                a.dirty = true;
                pushCue(a.game, Cue::BetChange);  // témoin sonore du volume
            } else if (a.menuIndex == 2 && a.roster.count > 0) {
                // Fait défiler les joueurs existants.
                syncPlayer(a.roster, a.game.machine.econ, a.game.spins, 0);
                const uint8_t n = a.roster.count;
                a.roster.current = static_cast<uint8_t>(
                    (a.roster.current + (inc ? 1 : n - 1)) % n);
                Player* p = currentPlayer(a.roster);
                a.game.machine.econ.credits = p->credits;
                a.game.spins = p->spins;
                clampBet(a.game.machine.econ);
                a.dirty = true;
            }
            break;
        }
        case AppKey::Confirm:
            if (a.menuIndex == 2) {
                // Nouveau joueur : passe par la saisie du nom.
                syncPlayer(a.roster, a.game.machine.econ, a.game.spins, 0);
                a.screen = AppScreen::NameEntry;
            } else if (a.menuIndex == 3) {
                if (!a.resetArmed) {
                    a.resetArmed = true;  // première pression : on arme
                } else {
                    resetRoster(a.roster);   // seconde : on efface tout
                    a.resetArmed = false;
                    a.dirty = true;
                    a.game.machine.econ = freshEconomy();
                    a.game.spins = 0;
                    a.screen = AppScreen::NameEntry;
                }
            }
            break;
        case AppKey::Settings:
        case AppKey::Back:
            a.resetArmed = false;
            a.screen = AppScreen::Lobby;
            break;
        default:
            break;
    }
}

void keySlotSettings(App& a, AppKey k) {
    switch (k) {
        case AppKey::Left:
        case AppKey::Right:
        case AppKey::Confirm:
            // Ligne unique : GLYPHS geek ↔ classique.
            a.settings.slotSkin = a.settings.slotSkin ? 0 : 1;
            a.dirty = true;
            break;
        case AppKey::Settings:
        case AppKey::Back:
            a.screen = AppScreen::Slot;
            break;
        default:
            break;
    }
}

}  // namespace

void handleKey(App& a, AppKey k, uint32_t now, RngFn rng) {
    if (k == AppKey::None) return;
    switch (a.screen) {
        case AppScreen::NameEntry:
            // Enter est géré ici ; les caractères par feedNameChar.
            if (k == AppKey::Confirm) commitName(a);
            if (k == AppKey::Back && a.roster.count > 0) {
                a.nameEntry = NameEntry{};
                a.screen = AppScreen::Lobby;  // annulation possible s'il
            }                                 // existe déjà des joueurs
            break;
        case AppScreen::Lobby:
            keyLobby(a, k, now, rng);
            break;
        case AppScreen::Slot:
            keySlot(a, k, now, rng);
            break;
        case AppScreen::SlotHelp:
            if (k == AppKey::Help || k == AppKey::Back || k == AppKey::Confirm) {
                a.screen = a.helpReturn;  // on revient d'où l'on vient
            }
            break;
        case AppScreen::SlotSettings:
            keySlotSettings(a, k);
            break;
        case AppScreen::GlobalSettings:
            keyGlobalSettings(a, k);
            break;
        case AppScreen::Leaderboard:
            if (k == AppKey::Board || k == AppKey::Back || k == AppKey::Confirm) {
                a.screen = AppScreen::Lobby;
            }
            break;
    }
}

void feedNameChar(App& a, char c) {
    if (a.screen != AppScreen::NameEntry) return;
    if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
    const bool ok = (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9');
    if (!ok || a.nameEntry.len >= kNameMax) return;
    a.nameEntry.buf[a.nameEntry.len++] = c;
    a.nameEntry.buf[a.nameEntry.len] = '\0';
    a.nameEntry.rosterFull = false;
}

void nameBackspace(App& a) {
    if (a.screen != AppScreen::NameEntry || a.nameEntry.len == 0) return;
    a.nameEntry.buf[--a.nameEntry.len] = '\0';
    a.nameEntry.rosterFull = false;
}

void tickApp(App& a, uint32_t now, RngFn rng) {
    // Le jeu n'avance (et le mode démo ne s'arme) que sur son écran.
    if (a.screen != AppScreen::Slot) {
        noteInput(a.game, now);
        return;
    }
    updateGame(a.game, now, rng);
    // Un tour fini → l'entrée du joueur au classement est à jour.
    if (a.game.phase != Phase::Spinning && a.game.reelsStopped > 0) {
        syncAndMarkDirty(a);
        a.game.reelsStopped = 0;
    }
}

}  // namespace core
