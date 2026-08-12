#include "app.h"

namespace core {

// Le SOLDE est partagé — c'est ce qui fait un casino. La MISE ne l'est
// pas : chaque jeu garde la sienne, parce qu'elles n'ont pas la même
// portée (au format vidéo, une mise de 5 engage 25). Passer d'un jeu à
// l'autre ne doit pas changer l'enjeu à l'insu du joueur.
uint32_t demoDelayMs(const App& a) {
    const uint8_t i = a.settings.demoDelay < kDemoDelaySteps ? a.settings.demoDelay
                                                             : kDefaultDemoDelay;
    return static_cast<uint32_t>(kDemoDelays[i]) * 1000u;
}

Cue takeAppCue(App& a) {
    Cue c = takeCue(a.game);
    if (c == Cue::None) c = takeVideoCue(a.video);
    if (c == Cue::None) c = takeBjCue(a.bj);
    if (c == Cue::None) c = takeVpCue(a.poker);
    if (c == Cue::None) c = takeRltCue(a.roulette);
    return c;
}

bool appInDemo(const App& a) {
    switch (a.screen) {
        case AppScreen::Slot: return a.game.attract;
        case AppScreen::Video: return a.video.attract;
        case AppScreen::Blackjack: return a.bj.attract;
        case AppScreen::Poker: return a.poker.attract;
        case AppScreen::Roulette: return a.roulette.attract;
        default: return false;
    }
}

void beginBoot(App& a, uint32_t now) {
    a.afterBoot = a.screen;   // là où l'app allait avant l'intermède
    a.bootT0 = now;
    a.lastInputMs = now;
    a.screen = a.settings.bootFx ? AppScreen::Boot : a.afterBoot;
}

void pushEconomy(App& a) {
    a.game.machine.econ.credits = a.econ.credits;
    a.video.econ.credits = a.econ.credits;
    a.bj.econ.credits = a.econ.credits;
    a.poker.econ.credits = a.econ.credits;
    a.roulette.econ.credits = a.econ.credits;
    // Chaque mise reste dans son jeu, ramenée à ce que le solde permet.
    clampBet(a.game.machine.econ);
    clampBetFor(a.video.econ, kVideoLines);
    clampBet(a.bj.econ);
    clampBet(a.poker.econ);
    clampBet(a.roulette.econ);
}

// La mise vit DANS le joueur : plus de table parallèle à garder
// synchrone, et changer de joueur change ses mises sans un mot de code
// supplémentaire.
void storePlayerBets(App& a) {
    Player* p = currentPlayer(a.roster);
    if (!p) return;
    p->bet[0] = a.game.machine.econ.betIndex;
    p->bet[1] = a.video.econ.betIndex;
    p->bet[2] = a.bj.econ.betIndex;
    p->bet[3] = a.poker.econ.betIndex;
    p->bet[4] = a.roulette.econ.betIndex;
}

void loadPlayerBets(App& a) {
    Player* p = currentPlayer(a.roster);
    if (!p) return;
    auto pick = [](uint8_t v) { return v < kBetSteps ? v : kDefaultBetIndex; };
    a.game.machine.econ.betIndex = pick(p->bet[0]);
    a.video.econ.betIndex = pick(p->bet[1]);
    a.bj.econ.betIndex = pick(p->bet[2]);
    a.poker.econ.betIndex = pick(p->bet[3]);
    a.roulette.econ.betIndex = pick(p->bet[4]);
    pushEconomy(a);  // ramène chaque mise à ce que le solde permet
}

uint16_t betOfGame(const App& a, GameId g) {
    switch (g) {
        case GameId::Video: return bet(a.video.econ);
        case GameId::Blackjack: return bet(a.bj.econ);
        case GameId::Poker: return bet(a.poker.econ);
        case GameId::Roulette: return bet(a.roulette.econ);
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
        case AppScreen::Poker:
        case AppScreen::PokerHelp:
            a.econ.credits = a.poker.econ.credits;
            break;
        case AppScreen::Roulette:
        case AppScreen::RouletteHelp:
            a.econ.credits = a.roulette.econ.credits;
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
    a.poker = newVpSession(now);
    a.roulette = newRouletteSession(now, rng);
    a.econ = freshEconomy();
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
    return a.game.spins + a.video.spins + a.bj.hands + a.poker.hands +
           a.roulette.spins;
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
        case GameId::Poker: return AppScreen::Poker;
        case GameId::Roulette: return AppScreen::Roulette;
        default: return AppScreen::Slot;
    }
}

AppScreen helpOf(GameId g) {
    switch (g) {
        case GameId::Video: return AppScreen::VideoHelp;
        case GameId::Blackjack: return AppScreen::BjHelp;
        case GameId::Poker: return AppScreen::PokerHelp;
        case GameId::Roulette: return AppScreen::RouletteHelp;
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
            a.helpPage = 0;
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
            a.helpPage = 0;
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
            a.helpPage = 0;
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
            a.helpPage = 0;
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

void keyPoker(App& a, AppKey k, uint32_t now, RngFn rng) {
    switch (k) {
        case AppKey::Confirm:
            if (a.poker.phase == VpPhase::Holding) {
                vpConfirm(a.poker, now, rng);
                if (a.poker.phase == VpPhase::Result) {
                    syncAndMarkDirty(a, a.poker.payout);
                }
            } else if (vpDeal(a.poker, now, rng)) {
                syncAndMarkDirty(a, 0);
            }
            break;
        case AppKey::Left:
        case AppKey::Right:
            if (a.poker.phase == VpPhase::Holding) {
                vpMoveCursor(a.poker, k == AppKey::Left ? -1 : 1);
            } else {
                // Hors main : les flèches règlent la mise, comme ailleurs.
                if (k == AppKey::Left) lowerBet(a.poker.econ);
                else raiseBet(a.poker.econ);
                pushVpCue(a.poker, Cue::BetChange);
            }
            break;
        case AppKey::Help:
            a.helpReturn = AppScreen::Poker;
            a.helpPage = 0;
            a.screen = AppScreen::PokerHelp;
            break;
        case AppKey::Back:
            // On ne quitte pas au milieu d'une main : la mise est engagée.
            if (a.poker.phase != VpPhase::Holding) leaveGame(a);
            break;
        default:
            break;
    }
}

void keyRoulette(App& a, AppKey k, uint32_t now, RngFn rng) {
    switch (k) {
        case AppKey::Confirm:
            if (rltSpin(a.roulette, now, rng)) syncAndMarkDirty(a, 0);
            break;
        case AppKey::Left:  rltCycleBet(a.roulette, -1); break;
        case AppKey::Right: rltCycleBet(a.roulette, +1); break;
        case AppKey::Up:
            // Sur un plein, les flèches verticales choisissent le numéro ;
            // ailleurs elles règlent la mise.
            if (a.roulette.kind == BetKind::Straight) rltCycleNumber(a.roulette, +1);
            else if (a.roulette.phase != RltPhase::Spinning) raiseBet(a.roulette.econ);
            break;
        case AppKey::Down:
            if (a.roulette.kind == BetKind::Straight) rltCycleNumber(a.roulette, -1);
            else if (a.roulette.phase != RltPhase::Spinning) lowerBet(a.roulette.econ);
            break;
        case AppKey::Help:
            a.helpReturn = AppScreen::Roulette;
            a.helpPage = 0;
            a.screen = AppScreen::RouletteHelp;
            break;
        case AppKey::Back:
            if (a.roulette.phase != RltPhase::Spinning) leaveGame(a);
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
            if (a.menuIndex == RowSound) {
                a.settings.muted = a.settings.muted ? 0 : 1;
                a.dirty = true;
            } else if (a.menuIndex == RowVolume) {
                if (inc && a.settings.volume < 3) ++a.settings.volume;
                if (!inc && a.settings.volume > 0) --a.settings.volume;
                a.dirty = true;
                pushCue(a.game, Cue::BetChange);
            } else if (a.menuIndex == RowDemo) {
                a.settings.demoOn = a.settings.demoOn ? 0 : 1;
                a.dirty = true;
            } else if (a.menuIndex == RowDemoDelay) {
                if (inc && a.settings.demoDelay + 1 < kDemoDelaySteps) ++a.settings.demoDelay;
                if (!inc && a.settings.demoDelay > 0) --a.settings.demoDelay;
                a.dirty = true;
            } else if (a.menuIndex == RowBootFx) {
                a.settings.bootFx = a.settings.bootFx ? 0 : 1;
                a.dirty = true;
            } else if (a.menuIndex == RowPlayer && a.roster.count > 0) {
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
            if (a.menuIndex == RowPlayer) {
                storePlayerBets(a);
                syncPlayer(a.roster, a.econ, totalSpins(a), 0);
                a.screen = AppScreen::NameEntry;
            } else if (a.menuIndex == RowReset) {
                if (!a.resetArmed) {
                    a.resetArmed = true;
                } else {
                    resetRoster(a.roster);
                    a.resetArmed = false;
                    a.dirty = true;
                    a.econ = freshEconomy();
                    a.game.spins = 0;
                    a.video.spins = 0;
                    a.bj.hands = 0;
                    a.poker.hands = 0;
                    a.roulette.spins = 0;
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

uint8_t helpPageCount(AppScreen help) {
    switch (help) {
        case AppScreen::SlotHelp: return 2;   // gains, règles
        case AppScreen::PokerHelp: return 2;  // gains, règles
        case AppScreen::RouletteHelp: return 2;
        case AppScreen::VideoHelp: return 3;  // gains, lignes, règles
        case AppScreen::BjHelp: return 2;     // règles, actions
        default: return 1;
    }
}

void handleKey(App& a, AppKey k, uint32_t now, RngFn rng) {
    if (k == AppKey::None) return;
    // Un geste, n'importe lequel, désarme la démo partout.
    a.lastInputMs = now;
    switch (a.screen) {
        case AppScreen::Boot:
            // N'importe quelle touche saute l'intermède : on ne fait pas
            // attendre quelqu'un qui sait déjà ce qu'il veut.
            a.screen = a.afterBoot;
            break;
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
        case AppScreen::Poker: keyPoker(a, k, now, rng); break;
        case AppScreen::Roulette: keyRoulette(a, k, now, rng); break;

        case AppScreen::SlotHelp:
        case AppScreen::VideoHelp:
        case AppScreen::BjHelp:
        case AppScreen::PokerHelp:
        case AppScreen::RouletteHelp: {
            const uint8_t pages = helpPageCount(a.screen);
            if (k == AppKey::Down && a.helpPage + 1 < pages) {
                ++a.helpPage;
            } else if (k == AppKey::Up && a.helpPage > 0) {
                --a.helpPage;
            } else if (k == AppKey::Help || k == AppKey::Back ||
                       k == AppKey::Confirm) {
                a.helpPage = 0;
                a.screen = a.helpReturn;
            }
            break;
        }

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

namespace {

// Stratégie de démo, volontairement simple et lisible : la démo montre le
// jeu, elle ne cherche pas à bien jouer.
void driveDemo(App& a, uint32_t now, RngFn rng) {
    switch (a.screen) {
        case AppScreen::Blackjack:
            if (a.bj.bj.phase == BjPhase::PlayerTurn && a.bj.revealed >= 4) {
                // Une décision toutes les 700 ms, pour qu'on la voie.
                if (now - a.bj.lastInputMs < 700) break;
                a.bj.lastInputMs = now;
                a.bj.choice = handValue(a.bj.bj.player).total < 17
                    ? BjChoice::Hit : BjChoice::Stand;
                bjConfirm(a.bj, now, rng);
            } else if (a.bj.bj.phase == BjPhase::Idle ||
                       a.bj.bj.phase == BjPhase::Settle) {
                if (now - a.bj.phaseT0 < 2200) break;
                bjStartHand(a.bj, now, rng, /*byPlayer=*/false);
            }
            break;
        case AppScreen::Poker:
            if (a.poker.phase == VpPhase::Holding &&
                vpVisible(a.poker) >= kPokerHandSize) {
                if (now - a.poker.phaseT0 < 1400) break;
                // Garde les cartes dont le rang est apparié : la règle la
                // plus simple qui produise un jeu crédible à regarder.
                for (uint8_t i = 0; i < kPokerHandSize; ++i) {
                    a.poker.held[i] = false;
                    for (uint8_t j = 0; j < kPokerHandSize; ++j) {
                        if (i != j && a.poker.hand.c[i].rank == a.poker.hand.c[j].rank) {
                            a.poker.held[i] = true;
                        }
                    }
                }
                a.poker.cursor = kVpDrawSlot;
                vpConfirm(a.poker, now, rng);
            } else if (a.poker.phase != VpPhase::Holding) {
                if (now - a.poker.phaseT0 < 2600) break;
                vpDeal(a.poker, now, rng, /*byPlayer=*/false);
            }
            break;
        case AppScreen::Roulette:
            if (a.roulette.phase != RltPhase::Idle) break;
            if (now - a.roulette.phaseT0 < 1200) break;
            // Un pari différent à chaque tour : la démo montre l'éventail.
            a.roulette.kind = static_cast<BetKind>(drawBelow(rng, kBetKinds));
            if (a.roulette.kind == BetKind::Straight) {
                a.roulette.straight = static_cast<uint8_t>(drawBelow(rng, kPockets));
            }
            rltSpin(a.roulette, now, rng, /*byPlayer=*/false);
            break;
        default:
            break;
    }
}

}  // namespace

void tickApp(App& a, uint32_t now, RngFn rng) {
    if (a.screen == AppScreen::Boot) {
        if (now - a.bootT0 >= kBootTotalMs) a.screen = a.afterBoot;
        a.lastInputMs = now;  // pas de démo pendant l'allumage
        return;
    }
    // Armement commun : un seul compteur d'inactivité pour tout l'objet.
    const bool armed = a.settings.demoOn != 0 &&
                       now - a.lastInputMs >= demoDelayMs(a);
    a.game.demoArmed = armed;
    a.video.demoArmed = armed;
    if (armed) {
        driveDemo(a, now, rng);
    } else {
        // Désarmé : les jeux à rouleaux repartent en couleurs.
        noteInput(a.game, now);
        noteVideoInput(a.video, now);
        a.bj.attract = false;
        a.poker.attract = false;
        a.roulette.attract = false;
    }

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
        case AppScreen::Poker:
            vpUpdate(a.poker, now, rng);
            break;
        case AppScreen::Roulette: {
            const RltPhase before = a.roulette.phase;
            rltUpdate(a.roulette, now);
            if (before == RltPhase::Spinning && a.roulette.phase != before) {
                syncAndMarkDirty(a, a.roulette.payout);
            }
            break;
        }
        case AppScreen::Blackjack: {
            const BjPhase before = a.bj.bj.phase;
            bjUpdate(a.bj, now, rng);
            if (before != BjPhase::Settle && a.bj.bj.phase == BjPhase::Settle) {
                syncAndMarkDirty(a, a.bj.bj.payout);
            }
            break;
        }
        default:
            // Hors jeu (menus, aide), rien ne s'anime.
            noteInput(a.game, now);
            noteVideoInput(a.video, now);
            break;
    }
}

}  // namespace core
