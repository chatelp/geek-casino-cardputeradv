// La trace d'oscilloscope du bas d'écran. Écrite en fonctions pures, elle
// se teste comme une table de gains : on interroge une colonne à un
// instant, sans rien faire avancer.
//
// Le test qui compte vraiment est celui de l'AMPLITUDE. Le registre qui
// précédait (un analyseur de spectre) est mort exactement là : ses barres
// au repos occupaient déjà toute la hauteur, donc l'à-coup ne se voyait
// pas. Ici, le rapport entre le repos et la salve est vérifié.
#include <unity.h>

#include "scope.h"

void setUp() {}
void tearDown() {}

namespace {

constexpr int kW = 240;
core::ReelMotion kReels[3];

void armThree() {
    const uint32_t dur[3] = {800, 1200, 1600};
    for (int i = 0; i < 3; ++i) {
        kReels[i] = core::ReelMotion{};
        kReels[i].t0 = 0;
        kReels[i].dur = dur[i];
    }
}

int peakAmplitude(uint32_t now, const core::ScopeDrive& d) {
    int best = 0;
    for (int x = 0; x < kW; ++x) {
        const int v = core::scopeAt(x, kW, now, d);
        const int a = v < 0 ? -v : v;
        if (a > best) best = a;
    }
    return best;
}

}  // namespace

static void test_a_lock_is_far_louder_than_the_resting_trace() {
    armThree();
    // Avant tout verrouillage : la trace ne fait que respirer.
    const int rest = peakAmplitude(400, core::scopeDriveOfReels(kReels, 3, 400));
    // 40 ms après le premier verrouillage, la salve est encore à l'écran.
    const int burst = peakAmplitude(840, core::scopeDriveOfReels(kReels, 3, 840));

    TEST_ASSERT_TRUE_MESSAGE(rest <= 40, "le repos doit rester plat");
    TEST_ASSERT_TRUE_MESSAGE(burst > 90, "la salve doit saturer");
    // Le rapport EST la fonctionnalité : c'est lui qui manquait au registre
    // précédent, où repos et pointe se ressemblaient.
    TEST_ASSERT_TRUE_MESSAGE(burst > rest * 2, "amplitude insuffisante");
}

static void test_a_burst_travels_across_the_screen() {
    armThree();
    // On suit la colonne du maximum : elle doit remonter vers la gauche.
    auto peakColumn = [](uint32_t now) {
        const core::ScopeDrive d = core::scopeDriveOfReels(kReels, 3, now);
        int best = 0, bx = 0;
        for (int x = 0; x < kW; ++x) {
            const int v = core::scopeAt(x, kW, now, d);
            const int a = v < 0 ? -v : v;
            if (a > best) { best = a; bx = x; }
        }
        return bx;
    };
    const int early = peakColumn(830);
    const int later = peakColumn(1030);
    TEST_ASSERT_TRUE(early > kW - 30);   // la salve naît au bord droit
    TEST_ASSERT_TRUE(later < early - 30);  // et s'en éloigne
}

static void test_the_trace_calms_as_reels_lock() {
    armThree();
    const uint8_t all = core::scopeDriveOfReels(kReels, 3, 400).energy;
    const uint8_t one = core::scopeDriveOfReels(kReels, 3, 1400).energy;
    const uint8_t none = core::scopeDriveOfReels(kReels, 3, 2000).energy;
    TEST_ASSERT_EQUAL_UINT8(100, all);
    TEST_ASSERT_TRUE(one < all);
    TEST_ASSERT_TRUE(one > 40);  // le dernier rouleau reste tendu
    TEST_ASSERT_EQUAL_UINT8(0, none);
}

static void test_every_lock_gets_its_own_burst() {
    armThree();
    // Une fois les trois rouleaux arrêtés, les trois salves existent —
    // même si elles ont fini de défiler.
    TEST_ASSERT_EQUAL_UINT8(3, core::scopeDriveOfReels(kReels, 3, 1700).bursts);
    TEST_ASSERT_EQUAL_UINT8(1, core::scopeDriveOfReels(kReels, 3, 900).bursts);
    TEST_ASSERT_EQUAL_UINT8(0, core::scopeDriveOfReels(kReels, 3, 400).bursts);
}

static void test_the_trace_never_leaves_its_range() {
    armThree();
    for (uint32_t t = 0; t < 2200; t += 13) {
        const core::ScopeDrive d = core::scopeDriveOfReels(kReels, 3, t);
        for (int x = 0; x < kW; x += 3) {
            const int v = core::scopeAt(x, kW, t, d);
            TEST_ASSERT_TRUE(v <= 100 && v >= -100);
        }
    }
}

static void test_the_trace_is_reproducible() {
    // Aucun état, aucune horloge : deux interrogations du même instant
    // donnent la même trace. C'est ce qui rend les captures comparables.
    armThree();
    const core::ScopeDrive d = core::scopeDriveOfReels(kReels, 3, 900);
    for (int x = 0; x < kW; x += 7) {
        TEST_ASSERT_EQUAL_INT8(core::scopeAt(x, kW, 900, d),
                               core::scopeAt(x, kW, 900, d));
    }
}

static void test_the_celebration_escalates_with_the_tier() {
    // Escalier de D-008 : le nombre de salves suit le palier. Un petit gain
    // fait une secousse, un jackpot en fait cinq.
    uint8_t prev = 0;
    for (uint8_t tier = 1; tier <= 4; ++tier) {
        const uint8_t n = core::scopeDriveOfWin(tier, 5000).bursts;
        TEST_ASSERT_TRUE(n >= prev);
        prev = n;
    }
    TEST_ASSERT_EQUAL_UINT8(0, core::scopeDriveOfWin(0, 5000).bursts);
    TEST_ASSERT_EQUAL_UINT8(5, core::scopeDriveOfWin(4, 5000).bursts);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_a_lock_is_far_louder_than_the_resting_trace);
    RUN_TEST(test_a_burst_travels_across_the_screen);
    RUN_TEST(test_the_trace_calms_as_reels_lock);
    RUN_TEST(test_every_lock_gets_its_own_burst);
    RUN_TEST(test_the_trace_never_leaves_its_range);
    RUN_TEST(test_the_trace_is_reproducible);
    RUN_TEST(test_the_celebration_escalates_with_the_tier);
    return UNITY_END();
}
