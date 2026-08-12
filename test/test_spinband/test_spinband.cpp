// L'analyseur de spectre du bas d'écran. Il est écrit en fonctions pures,
// donc il se teste comme une table de gains : on interroge un instant
// précis, sans rien faire avancer.
#include <unity.h>

#include "spinband.h"

void setUp() {}
void tearDown() {}

namespace {

// Trois rouleaux armés à 0, qui s'arrêtent à 800, 1200 et 1600 ms — la
// cascade habituelle d'un tour.
core::ReelMotion kReels[3];

void armThree() {
    const uint32_t dur[3] = {800, 1200, 1600};
    for (int i = 0; i < 3; ++i) {
        kReels[i] = core::ReelMotion{};
        kReels[i].t0 = 0;
        kReels[i].dur = dur[i];
    }
}

}  // namespace

static void test_the_band_calms_down_as_reels_lock() {
    armThree();
    const uint8_t all = core::bandDriveOfReels(kReels, 3, 400).energy;
    const uint8_t two = core::bandDriveOfReels(kReels, 3, 1000).energy;
    const uint8_t one = core::bandDriveOfReels(kReels, 3, 1400).energy;
    const uint8_t none = core::bandDriveOfReels(kReels, 3, 2000).energy;
    TEST_ASSERT_EQUAL_UINT8(100, all);
    TEST_ASSERT_TRUE(two < all);
    TEST_ASSERT_TRUE(one < two);
    // Le dernier rouleau est le moment le plus tendu du tour : l'entrain se
    // concentre, il ne s'éteint pas.
    TEST_ASSERT_TRUE(one > 50);
    TEST_ASSERT_EQUAL_UINT8(0, none);
}

static void test_a_locking_reel_slams_every_bar_to_the_top() {
    armThree();
    // 10 ms après le verrouillage du premier rouleau.
    const core::BandDrive hit = core::bandDriveOfReels(kReels, 3, 810);
    for (uint8_t b = 0; b < core::kBandBars; ++b) {
        TEST_ASSERT_TRUE(core::bandLevel(b, 810, hit) > 90);
    }
    // 300 ms plus tard le coup est retombé : les barres du bord doivent
    // être redescendues, sinon le bandeau reste collé au plafond.
    const core::BandDrive after = core::bandDriveOfReels(kReels, 3, 1110);
    TEST_ASSERT_TRUE(core::bandLevel(0, 1110, after) < 90);
}

static void test_the_band_never_leaves_its_range() {
    armThree();
    for (uint32_t t = 0; t < 2000; t += 17) {
        const core::BandDrive d = core::bandDriveOfReels(kReels, 3, t);
        for (uint8_t b = 0; b < core::kBandBars; ++b) {
            const uint8_t lv = core::bandLevel(b, t, d);
            const uint8_t pk = core::bandPeak(b, t, d);
            TEST_ASSERT_TRUE(lv <= 100);
            TEST_ASSERT_TRUE(pk <= 100);
            // Le témoin de crête est un maximum : il ne peut pas passer
            // SOUS la barre qu'il coiffe.
            TEST_ASSERT_TRUE(pk >= lv);
        }
    }
}

static void test_the_middle_bars_stand_taller_than_the_edges() {
    // L'enveloppe en cloche est ce qui donne une forme au bandeau. Sans
    // elle il ne reste qu'un mur de bruit, où l'œil ne lit aucune
    // intensité. Comparé en moyenne : une barre isolée peut toujours tomber
    // sur un tirage bas.
    armThree();
    uint32_t mid = 0, edge = 0;
    for (uint32_t t = 0; t < 700; t += 11) {
        const core::BandDrive d = core::bandDriveOfReels(kReels, 3, t);
        mid += core::bandLevel(core::kBandBars / 2, t, d);
        edge += core::bandLevel(0, t, d);
    }
    TEST_ASSERT_TRUE(mid > edge);
}

static void test_the_band_is_reproducible() {
    // Aucun état, aucune horloge interne : deux interrogations du même
    // instant donnent la même image. C'est ce qui rend les captures
    // comparables d'une exécution à l'autre.
    armThree();
    const core::BandDrive d = core::bandDriveOfReels(kReels, 3, 512);
    for (uint8_t b = 0; b < core::kBandBars; ++b) {
        TEST_ASSERT_EQUAL_UINT8(core::bandLevel(b, 512, d),
                                core::bandLevel(b, 512, d));
    }
    // Et deux images voisines du même palier sont identiques : le spectre
    // se recalcule par paliers, pas à chaque image — sinon il grésille.
    for (uint8_t b = 0; b < core::kBandBars; ++b) {
        TEST_ASSERT_EQUAL_UINT8(core::bandLevel(b, 500, d),
                                core::bandLevel(b, 500 + core::kBandStepMs / 3, d));
    }
}

static void test_the_celebration_band_escalates_with_the_tier() {
    // Même escalier que le son et l'animation (D-008) : un petit gain ne
    // doit pas faire le même bruit visuel qu'un jackpot.
    uint8_t prev = 0;
    for (uint8_t tier = 1; tier <= 4; ++tier) {
        const uint8_t e = core::bandDriveOfWin(tier).energy;
        TEST_ASSERT_TRUE(e > prev);
        prev = e;
    }
    TEST_ASSERT_EQUAL_UINT8(0, core::bandDriveOfWin(0).energy);
    TEST_ASSERT_EQUAL_UINT8(100, core::bandDriveOfWin(4).energy);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_the_band_calms_down_as_reels_lock);
    RUN_TEST(test_a_locking_reel_slams_every_bar_to_the_top);
    RUN_TEST(test_the_band_never_leaves_its_range);
    RUN_TEST(test_the_middle_bars_stand_taller_than_the_edges);
    RUN_TEST(test_the_band_is_reproducible);
    RUN_TEST(test_the_celebration_band_escalates_with_the_tier);
    return UNITY_END();
}
