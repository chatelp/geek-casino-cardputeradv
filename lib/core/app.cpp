#include "app.h"

namespace core {

// Le SOLDE est partagé — c'est ce qui fait un casino. La MISE ne l'est
// pas : chaque jeu garde la sienne, parce qu'elles n'ont pas la même
// portée (au format vidéo, une mise de 5 engage 25). Passer d'un jeu à
// l'autre ne doit pas changer l'enjeu à l'insu du joueur.
void pushEconomy(App& a) {
    a.game.machine.econ.credits = a.econ.credits;
    a.video.econ.credits = a.econ.credits;
    a.bj.econ.credits = a.econ.credits;
    // Chaque mise reste dans son jeu, ramenée à ce que le solde permet.
    clampBet(a.game.machine.econ);
    clampBetFor(a.video.econ, kVideoLines);
    clampBet(a.bj.econ);
}

void storePlayerBets(App& a) {
    if (a.roster.count == 0 || a.roster.current >= kMaxPlayers) return;
    uint8_t* row = a.bets.bet[a.roster.current];
    row[0] = a.game.machine.econ.betIndex;
    row[1] = a.video.econ.betIndex;
    row[2] = a.bj.econ.betIndex;
    a.bets = makeBets(a.bets);  // somme de contrôle à jour
}

void loadPlayerBets(App& a) {
    if (a.roster.count == 0 || a.roster.current >= kMaxPlayers) return;
    const uint8_t* row = a.bets.bet[a.roster.current];
    a.game.machine.econ.betIndex = row[0] < kBetSteps ? row[0] : kDefaultBetIndex;
    a.video.econ.betIndex = row[1] < kBetSteps ? row[1] : kDefaultBetIndex;
    a.bj.econ.betIndex = row[2] < kBetSteps ? row[2] : kDefaultBetIndex;
    pushEconomy(a);  // ramène chaque mise à ce que le solde permet
}

uint16_t betOfGame(const App& a, GameId g) {
    switch (g) {
        case GameId::Video: return bet(a.video.econ);
        case GameId::Blackjack: return bet(a.bj.econ);
        default: return bet(a.game.machine.econ);
    }
}

void pullEconomy(App& a) {
    // Seul le solde remonte : la mise du jeu qu'on quitte lui appartient.
    switch (a.screen) {
        case AppScreen::Slot:
        case AppScreen::SlotHelp:
        case AppScreen::SlotSettings:
            a.econ.credits = a.game.machine.econ.credits;
            break;
        case AppScreen::Video:
        case AppScreen::VideoHelp:
        case AppScreen::VideoSettings:
            a.econ.credits = a.video.econ.credits;
            break;
        case AppScreen::Blackjack:
        case AppScreen::BjHelp:
        case AppScreen::BjSettings:
            a.econ.credits = a.bj.econ.credits;
            break;
        default:
            break;
    }
}

App newApp(uint32_t now, RngFn rng) {
    App a;
    a.game = newGame(now, rng);
    a.video = newVideoGame(now, rng);
    a.bj = newBjSession(now);
    a.econ = freshEconomy();
    a.bets = freshBets();
    pushEconomy(a);
    return a;
}

void enterFromSave(App& a) {
    Player* p = currentPlayer(a.roster);
    if (!p) {
        a.screen = AppScreen::NameEntry;
        return;
    }
    a.econ.credits = p->credits;
    a.game.spins = p->spins;
    loadPlayerBets(a);
    a.screen = AppScreen::Lobby;
}

namespace {

uint32_t totalSpins(const App& a) {
    return a.game.spins + a.video.spins + a.bj.hands;
}

void syncAndMarkDirty(App& a, uint32_t payout = 0) {
    pullEconomy(a);
    syncPlayer(a.roster, a.econ, totalSpins(a), payout);
    storePlayerBets(a);
    pushEconomy(a);
    a.dirty = true;
}

void commitName(App& a) {
    if (a.nameEntry.len == 0) return;
    if (!addOrSwitchPlayer(a.roster, a.nameEntry.buf)) {
        a.nameEntry.rosterFull = true;
        return;
    }
    Player* p = currentPlayer(a.roster);
    a.econ.credits = p->credits;
    a.game.spins = p->spins;
    loadPlayerBets(a);  // le nouveau joueur retrouve SES mises
    a.nameEntry = NameEntry{};
    a.dirty = true;
    a.screen = AppScreen::Lobby;
}

AppScreen screenOf(GameId g) {
    switch (g) {
        case GameId::Video: return AppScreen::Video;
        case GameId::Blackjack: return AppScreen::Blackjack;
        default: return AppScreen::Slot;
    }
}

AppScreen helpOf(GameId g) {
    switch (g) {
        case GameId::Video: return AppScreen::VideoHelp;
        case GameId::Blackjack: return AppScreen::BjHelp;
        default: return AppScreen::SlotHelp;
    }
}

void keyLobby(App& a, AppKey k, uint32_t now, RngFn rng) {
    switch (k) {
        case AppKey::Up:
            if (a.lobbyIndex > 0) --a.lobbyIndex;
            break;
        case AppKey::Down:
            if (a.lobbyIndex + 1 < kGameCount) ++a.lobbyIndex;
            break;
        case AppKey::Confirm:
            pushEconomy(a);
            noteInput(a.game, now);
            noteVideoInput(a.video, now);
            a.screen = screenOf(static_cast<GameId>(a.lobbyIndex));
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
            a.helpReturn = AppScreen::Lobby;
            a.screen = helpOf(static_cast<GameId>(a.lobbyIndex));
            break;
        case AppKey::Back:
            a.quitRequested = true;
            break;
        default:
            break;
    }
}

void leaveGame(App& a) {
    syncAndMarkDirty(a);
    a.screen = AppScreen::Lobby;
}

void keySlot(App& a, AppKey k, uint32_t now, RngFn rng) {
    switch (k) {
        case AppKey::Confirm:
            noteInput(a.game, now);
            if (startSpin(a.game, now, rng)) {
                ++a.game.spins;
                syncAndMarkDirty(a, a.game.outcome.payout);
            }
            break;
        case AppKey::Left:
        case AppKey::Right:
            noteInput(a.game, now);
            // Mise figée pendant la rotation : la changer en vol reviendrait
            // à modifier l'enjeu après avoir vu une partie du résultat.
            if (a.game.phase == Phase::Spinning) break;
            if (k == AppKey::Left) lowerBet(a.game.machine.econ);
            else raiseBet(a.game.machine.econ);
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
            leaveGame(a);
            break;
        default:
            noteInput(a.game, now);
            break;
    }
}

void keyVideo(App& a, AppKey k, uint32_t now, RngFn rng) {
    switch (k) {
        case AppKey::Confirm:
            noteVideoInput(a.video, now);
            if (startVideoSpin(a.video, now, rng)) {
                syncAndMarkDirty(a, a.video.payout);
            }
            break;
        case AppKey::Left:
        case AppKey::Right:
            noteVideoInput(a.video, now);
            if (a.video.phase == Phase::Spinning) break;
            // Le format vidéo engage cinq mises : la montée en tient compte.
            if (k == AppKey::Left) lowerBet(a.video.econ);
            else raiseBetFor(a.video.econ, kVideoLines);
            pushVideoCue(a.video, Cue::BetChange);
            break;
        case AppKey::Help:
            a.helpReturn = AppScreen::Video;
            a.screen = AppScreen::VideoHelp;
            break;
        case AppKey::Settings:
            a.menuIndex = 0;
            a.screen = AppScreen::VideoSettings;
            break;
        case AppKey::Back:
            leaveGame(a);
            break;
        default:
            noteVideoInput(a.video, now);
            break;
    }
}

void keyBlackjack(App& a, AppKey k, uint32_t now, RngFn rng) {
    switch (k) {
        case AppKey::Confirm:
            if (a.bj.bj.phase == BjPhase::PlayerTurn) {
                bjConfirm(a.bj, now, rng);
            } else if (a.bj.bj.phase != BjPhase::DealerTurn) {
                // Entre deux mains : Espace distribue.
                if (bjStartHand(a.bj, now, rng)) syncAndMarkDirty(a, a.bj.bj.payout);
            }
            break;
        case AppKey::Left:
            if (a.bj.bj.phase == BjPhase::PlayerTurn) bjMoveChoice(a.bj, -1, now);
            else { lowerBet(a.bj.econ); pushBjCue(a.bj, Cue::BetChange); }
            break;
        case AppKey::Right:
            if (a.bj.bj.phase == BjPhase::PlayerTurn) bjMoveChoice(a.bj, +1, now);
            else { raiseBet(a.bj.econ); pushBjCue(a.bj, Cue::BetChange); }
            break;
        case AppKey::Help:
            a.helpReturn = AppScreen::Blackjack;
            a.screen = AppScreen::BjHelp;
            break;
        case AppKey::Settings:
            a.menuIndex = 0;
            a.screen = AppScreen::BjSettings;
            break;
        case AppKey::Back:
            // On ne quitte pas une main en cours : elle se solderait sans
            // que le joueur voie le résultat de sa mise.
            if (a.bj.bj.phase != BjPhase::PlayerTurn &&
                a.bj.bj.phase != BjPhase::DealerTurn) {
                leaveGame(a);
            }
            break;
        default:
            break;
    }
}

void keyGlobalSettings(App& a, AppKey k) {
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
                pushCue(a.game, Cue::BetChange);
            } else if (a.menuIndex == 2 && a.roster.count > 0) {
                storePlayerBets(a);   // les mises du joueur qui s'en va
                syncPlayer(a.roster, a.econ, totalSpins(a), 0);
                const uint8_t n = a.roster.count;
                a.roster.current = static_cast<uint8_t>(
                    (a.roster.current + (inc ? 1 : n - 1)) % n);
                Player* p = currentPlayer(a.roster);
                a.econ.credits = p->credits;
                loadPlayerBets(a);    // celles du joueur qui arrive
                a.dirty = true;
            }
            break;
        }
        case AppKey::Confirm:
            if (a.menuIndex == 2) {
                storePlayerBets(a);
                syncPlayer(a.roster, a.econ, totalSpins(a), 0);
                a.screen = AppScreen::NameEntry;
            } else if (a.menuIndex == 3) {
                if (!a.resetArmed) {
                    a.resetArmed = true;
                } else {
                    resetRoster(a.roster);
                    a.resetArmed = false;
                    a.dirty = true;
                    a.econ = freshEconomy();
                    a.bets = freshBets();
                    a.game.spins = 0;
                    a.video.spins = 0;
                    a.bj.hands = 0;
                    pushEconomy(a);
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

}  // namespace

void handleKey(App& a, AppKey k, uint32_t now, RngFn rng) {
    if (k == AppKey::None) return;
    switch (a.screen) {
        case AppScreen::NameEntry:
            if (k == AppKey::Confirm) commitName(a);
            if (k == AppKey::Back && a.roster.count > 0) {
                a.nameEntry = NameEntry{};
                a.screen = AppScreen::Lobby;
            }
            break;
        case AppScreen::Lobby: keyLobby(a, k, now, rng); break;
        case AppScreen::Slot: keySlot(a, k, now, rng); break;
        case AppScreen::Video: keyVideo(a, k, now, rng); break;
        case AppScreen::Blackjack: keyBlackjack(a, k, now, rng); break;

        case AppScreen::SlotHelp:
        case AppScreen::VideoHelp:
        case AppScreen::BjHelp:
            if (k == AppKey::Help || k == AppKey::Back || k == AppKey::Confirm) {
                a.screen = a.helpReturn;
            }
            break;

        case AppScreen::SlotSettings:
            if (k == AppKey::Left || k == AppKey::Right || k == AppKey::Confirm) {
                a.settings.slotSkin = a.settings.slotSkin ? 0 : 1;
                a.dirty = true;
            } else if (k == AppKey::Settings || k == AppKey::Back) {
                a.screen = AppScreen::Slot;
            }
            break;
        case AppScreen::VideoSettings:
            if (k == AppKey::Left || k == AppKey::Right || k == AppKey::Confirm) {
                a.settings.slotSkin = a.settings.slotSkin ? 0 : 1;
                a.dirty = true;
            } else if (k == AppKey::Settings || k == AppKey::Back) {
                a.screen = AppScreen::Video;
            }
            break;
        case AppScreen::BjSettings:
            if (k == AppKey::Left || k == AppKey::Right || k == AppKey::Confirm) {
                a.bj.hintsOn = !a.bj.hintsOn;
                a.dirty = true;
            } else if (k == AppKey::Settings || k == AppKey::Back) {
                a.screen = AppScreen::Blackjack;
            }
            break;

        case AppScreen::GlobalSettings: keyGlobalSettings(a, k); break;
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
    switch (a.screen) {
        case AppScreen::Slot:
            updateGame(a.game, now, rng);
            if (a.game.phase != Phase::Spinning && a.game.reelsStopped > 0) {
                a.game.reelsStopped = 0;
                syncAndMarkDirty(a, a.game.outcome.payout);
            }
            break;
        case AppScreen::Video:
            updateVideoGame(a.video, now, rng);
            if (a.video.phase != Phase::Spinning && a.video.reelsStopped > 0) {
                a.video.reelsStopped = 0;
                syncAndMarkDirty(a, a.video.payout);
            }
            break;
        case AppScreen::Blackjack: {
            const BjPhase before = a.bj.bj.phase;
            bjUpdate(a.bj, now, rng);
            if (before != BjPhase::Settle && a.bj.bj.phase == BjPhase::Settle) {
                syncAndMarkDirty(a, a.bj.bj.payout);
            }
            break;
        }
        default:
            // Hors jeu, le mode démo ne doit pas s'armer.
            noteInput(a.game, now);
            noteVideoInput(a.video, now);
            break;
    }
}

}  // namespace core
