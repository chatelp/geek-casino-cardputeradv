// Main appareil — seul fichier autorisé à toucher M5Cardputer.
// Aucune règle de jeu ici : il branche l'horloge, l'aléa, le clavier,
// l'IMU, le haut-parleur et la NVS sur le même core::App que le simulateur.
#ifdef ARDUINO

#include <M5Cardputer.h>
#include <Preferences.h>
#include <esp_random.h>

#include <cmath>

#include "app.h"
#include "hal_display.h"
#include "menus.h"
#include "rng.h"
#include "shake.h"
#include "sound.h"

// Constructeurs globaux : RIEN de M5, pas même `&M5Cardputer.Display` —
// Display est un membre-référence initialisé par le constructeur de M5, et
// l'ordre d'initialisation des globaux est indéfini. Le lire trop tôt
// stocke du garbage et pushSprite crashe au premier appel (mesuré sur
// l'appareil, backtrace à l'appui). La destination se passe au push.
static M5Canvas canvas;
static Preferences prefs;
static core::App app;
static core::ShakeDetector shake;
static uint32_t lastSaveMs = 0;

static uint32_t trng() { return esp_random(); }

// ------------------------------------------------------------------- audio
// Sinusoïdes à décroissance exponentielle, jouées d'un bloc par playRaw.
// Pas de tone() : le rendu est doux et le timbre maîtrisé (voir CLAUDE.md).
namespace {
constexpr size_t kAudioMax = core::kSampleRate * core::kMaxCueMs / 1000;
int16_t audioBuf[kAudioMax];

void playCue(core::Cue cue) {
    if (app.settings.muted || app.settings.volume == 0) return;
    const core::Cadence cd = core::cadenceOf(cue);
    if (cd.count == 0) return;

    size_t n = 0;
    for (uint8_t t = 0; t < cd.count; ++t) {
        const core::Tone& tone = cd.tones[t];
        const size_t len = core::kSampleRate * tone.ms / 1000;
        const float w = 2.0f * 3.14159265f * tone.hz / core::kSampleRate;
        for (size_t i = 0; i < len && n < kAudioMax; ++i, ++n) {
            // Décroissance exponentielle : le « clic » mécanique, pas le bip.
            const float env = std::exp(-5.0f * static_cast<float>(i) / len);
            audioBuf[n] = static_cast<int16_t>(std::sin(w * i) * env * 30000.0f);
        }
    }
    // Volume appliqué au moment de jouer, jamais à la construction.
    static const uint8_t vol[4] = {0, 96, 168, 255};
    M5Cardputer.Speaker.setVolume(vol[app.settings.volume]);
    M5Cardputer.Speaker.stop();  // un nouveau son interrompt le précédent
    M5Cardputer.Speaker.playRaw(audioBuf, n, core::kSampleRate, false, 1, 0);
}
}  // namespace

// ------------------------------------------------------------- persistance
static void loadFromNvs() {
    core::SaveData s{};
    if (prefs.getBytes("save", &s, sizeof(s)) == sizeof(s) &&
        core::applySave(s, app.roster, app.settings)) {
        // Clé séparée : une sauvegarde d'avant cette fonctionnalité se
        // charge normalement, avec les mises par défaut.
        core::BetMemory b{};
        if (prefs.getBytes("bets", &b, sizeof(b)) == sizeof(b) &&
            core::betsValid(b)) {
            app.bets = b;
        }
        core::enterFromSave(app);
    }
    // Sinon : premier lancement (ou sauvegarde corrompue rejetée en bloc)
    // → l'app démarre sur la saisie du nom.
}

static void saveToNvs(uint32_t now) {
    // Throttle : la NVS s'use, on n'écrit pas à chaque image.
    if (!app.dirty || app.game.phase == core::Phase::Spinning ||
        app.video.phase == core::Phase::Spinning) return;
    if (now - lastSaveMs < 2000) return;
    core::pullEconomy(app);
    core::syncPlayer(app.roster, app.econ,
                     app.game.spins + app.video.spins + app.bj.hands, 0);
    core::storePlayerBets(app);
    const core::SaveData s = core::makeSave(app.roster, app.settings);
    const core::BetMemory b = core::makeBets(app.bets);
    prefs.putBytes("save", &s, sizeof(s));
    prefs.putBytes("bets", &b, sizeof(b));
    app.dirty = false;
    lastSaveMs = now;
}

// ----------------------------------------------------------------- clavier
static void handleKeyboard(uint32_t now) {
    if (!M5Cardputer.Keyboard.isChange() || !M5Cardputer.Keyboard.isPressed()) return;
    const auto st = M5Cardputer.Keyboard.keysState();
    const bool naming = app.screen == core::AppScreen::NameEntry;

    if (st.enter) core::handleKey(app, core::AppKey::Confirm, now, trng);
    if (st.del && naming) core::nameBackspace(app);

    for (const auto c : st.word) {
        if (naming) {
            // Échap doit rester joignable PENDANT la saisie : sans ce cas,
            // la saisie avalait la touche et l'annulation était impossible.
            if (c == '`') {
                core::handleKey(app, core::AppKey::Back, now, trng);
            } else {
                core::feedNameChar(app, c);  // filtre A-Z 0-9 lui-même
            }
            continue;
        }
        switch (c) {
            case ' ': core::handleKey(app, core::AppKey::Confirm, now, trng); break;
            case ',': core::handleKey(app, core::AppKey::Left, now, trng); break;
            case '/': core::handleKey(app, core::AppKey::Right, now, trng); break;
            case ';': core::handleKey(app, core::AppKey::Up, now, trng); break;
            case '.': core::handleKey(app, core::AppKey::Down, now, trng); break;
            case 'h': core::handleKey(app, core::AppKey::Help, now, trng); break;
            case 's': core::handleKey(app, core::AppKey::Settings, now, trng); break;
            case 'l': core::handleKey(app, core::AppKey::Board, now, trng); break;
            case '`': core::handleKey(app, core::AppKey::Back, now, trng); break;
            default: break;
        }
    }
}

// --------------------------------------------------------------------- IMU
static void handleShake(uint32_t now) {
    // Le geste vaut pour les deux machines à sous, pas au blackjack.
    if (app.screen != core::AppScreen::Slot &&
        app.screen != core::AppScreen::Video) return;
    // L'IMU n'est pas ré-exposée par le wrapper Cardputer : API M5Unified.
    float ax = 0, ay = 0, az = 0;
    M5.Imu.update();
    M5.Imu.getAccel(&ax, &ay, &az);
    const float mag = std::sqrt(ax * ax + ay * ay + az * az);
    if (core::feedAccel(shake, mag, now)) {
        // Secouer = tirer le levier = la touche de confirmation du jeu.
        core::handleKey(app, core::AppKey::Confirm, now, trng);
    }
}

void setup() {
    auto cfg = M5.config();
    M5Cardputer.begin(cfg);
    M5Cardputer.Display.setRotation(1);
    canvas.setColorDepth(16);
    canvas.createSprite(hal::kScreenW, hal::kScreenH);

    prefs.begin("geekcasino", false);
    app = core::newApp(millis(), trng);
    loadFromNvs();
}

void loop() {
    M5Cardputer.update();
    const uint32_t now = millis();

    handleKeyboard(now);
    handleShake(now);
    core::tickApp(app, now, trng);

    // Les trois jeux ont leur file ; un seul est à l'écran à la fois.
    for (core::Cue c = core::takeCue(app.game); c != core::Cue::None;
         c = core::takeCue(app.game)) playCue(c);
    for (core::Cue c = core::takeVideoCue(app.video); c != core::Cue::None;
         c = core::takeVideoCue(app.video)) playCue(c);
    for (core::Cue c = core::takeBjCue(app.bj); c != core::Cue::None;
         c = core::takeBjCue(app.bj)) playCue(c);

    saveToNvs(now);

    ui::drawApp(canvas, app, now);
    canvas.pushSprite(&M5Cardputer.Display, 0, 0);
    delay(core::kFrameMs);
}

#endif  // ARDUINO
