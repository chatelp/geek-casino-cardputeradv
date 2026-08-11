// Le geste se règle par des tests, pas en secouant l'appareil au jugé.
#include <unity.h>

#include "shake.h"

void setUp() {}
void tearDown() {}

static void test_rest_never_triggers() {
    core::ShakeDetector d;
    for (uint32_t t = 0; t < 5000; t += 10) {
        // Appareil posé : ~1 g avec un peu de bruit de capteur.
        const float noise = ((t / 10) % 3) * 0.02f;
        TEST_ASSERT_FALSE(core::feedAccel(d, 1.0f + noise, t));
    }
}

static void test_a_shake_triggers_once() {
    core::ShakeDetector d;
    core::feedAccel(d, 1.0f, 0);  // amorçage au repos
    TEST_ASSERT_TRUE(core::feedAccel(d, 1.9f, 100));
    // La secousse continue : pas de re-déclenchement tant qu'on n'est pas
    // revenu au calme.
    TEST_ASSERT_FALSE(core::feedAccel(d, 2.1f, 120));
    TEST_ASSERT_FALSE(core::feedAccel(d, 1.8f, 140));
}

static void test_rearm_needs_calm_then_cooldown() {
    core::ShakeDetector d;
    core::feedAccel(d, 1.0f, 0);
    TEST_ASSERT_TRUE(core::feedAccel(d, 1.9f, 100));
    // Retour au calme immédiat, nouvelle secousse DANS le délai de garde :
    // refusée — une seule secousse énergique ne vaut pas deux tours.
    core::feedAccel(d, 1.0f, 200);
    TEST_ASSERT_FALSE(core::feedAccel(d, 1.9f, 300));
    // Après le délai : à nouveau permis.
    core::feedAccel(d, 1.0f, 900);
    TEST_ASSERT_TRUE(core::feedAccel(d, 1.9f, 1000));
}

static void test_freefall_also_counts() {
    // Une chute (norme vers 0) est un écart à 1 g comme un autre : le geste
    // « lancer vers le bas » déclenche aussi.
    core::ShakeDetector d;
    core::feedAccel(d, 1.0f, 0);
    TEST_ASSERT_TRUE(core::feedAccel(d, 0.2f, 100));
}

static void test_first_sample_never_triggers() {
    // Au démarrage, l'appareil peut être en mouvement : le premier
    // échantillon observe, il ne déclenche pas un tour fantôme.
    core::ShakeDetector d;
    TEST_ASSERT_FALSE(core::feedAccel(d, 2.5f, 0));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_rest_never_triggers);
    RUN_TEST(test_a_shake_triggers_once);
    RUN_TEST(test_rearm_needs_calm_then_cooldown);
    RUN_TEST(test_freefall_also_counts);
    RUN_TEST(test_first_sample_never_triggers);
    return UNITY_END();
}
