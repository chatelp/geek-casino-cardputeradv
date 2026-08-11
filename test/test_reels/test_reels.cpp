#include <unity.h>

#include "reels.h"

void setUp() {}
void tearDown() {}

static void test_strip_holds_every_symbol() {
    const core::ReelSet& rs = core::mvpReelSet();
    uint16_t total = 0;
    for (uint8_t s = 0; s < core::kSymbolCount; ++s) {
        const uint16_t n = core::countOn(rs, 0, s);
        TEST_ASSERT_TRUE_MESSAGE(n > 0, "un symbole absent de la bande ne sort jamais");
        total += n;
    }
    TEST_ASSERT_EQUAL_UINT16(core::kStripLen, total);
}

static void test_symbol_at_wraps_both_ways() {
    const core::ReelSet& rs = core::mvpReelSet();
    const uint8_t first = core::symbolAt(rs, 0, 0);
    TEST_ASSERT_EQUAL_UINT8(first, core::symbolAt(rs, 0, core::kStripLen));
    TEST_ASSERT_EQUAL_UINT8(first, core::symbolAt(rs, 0, 3 * core::kStripLen));
    // Le rouleau tourne aussi vers le haut : les positions négatives doivent
    // boucler proprement, sans indexer hors du tableau.
    TEST_ASSERT_EQUAL_UINT8(first, core::symbolAt(rs, 0, -core::kStripLen));
    TEST_ASSERT_EQUAL_UINT8(core::symbolAt(rs, 0, core::kStripLen - 1),
                            core::symbolAt(rs, 0, -1));
}

static void test_no_two_neighbours_are_identical() {
    // Invariant voulu : deux positions voisines diffèrent, sinon un arrêt
    // imprécis d'un cran changerait le résultat sans que l'œil le voie.
    const core::ReelSet& rs = core::mvpReelSet();
    for (int32_t i = 0; i < core::kStripLen; ++i) {
        TEST_ASSERT_NOT_EQUAL(core::symbolAt(rs, 0, i), core::symbolAt(rs, 0, i + 1));
    }
}

static void test_spin_stays_on_the_strip() {
    core::seedXorShift(7);
    const core::ReelSet& rs = core::mvpReelSet();
    uint16_t pos[core::kMaxReels];
    uint8_t sym[core::kMaxReels];
    for (int i = 0; i < 20000; ++i) {
        core::spin(rs, core::xorShift32, pos, sym);
        for (uint8_t r = 0; r < rs.reels; ++r) {
            TEST_ASSERT_LESS_THAN_UINT16(rs.len[r], pos[r]);
            TEST_ASSERT_LESS_THAN_UINT8(core::kSymbolCount, sym[r]);
            TEST_ASSERT_EQUAL_UINT8(core::symbolAt(rs, r, pos[r]), sym[r]);
        }
    }
}

static void test_spin_visits_every_position() {
    // Une position jamais tirée serait un biais invisible en jeu.
    core::seedXorShift(99);
    const core::ReelSet& rs = core::mvpReelSet();
    bool seen[core::kStripLen] = {false};
    uint16_t pos[core::kMaxReels];
    uint8_t sym[core::kMaxReels];
    for (int i = 0; i < 20000; ++i) {
        core::spin(rs, core::xorShift32, pos, sym);
        seen[pos[0]] = true;
    }
    for (int i = 0; i < core::kStripLen; ++i) TEST_ASSERT_TRUE(seen[i]);
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_strip_holds_every_symbol);
    RUN_TEST(test_symbol_at_wraps_both_ways);
    RUN_TEST(test_no_two_neighbours_are_identical);
    RUN_TEST(test_spin_stays_on_the_strip);
    RUN_TEST(test_spin_visits_every_position);
    return UNITY_END();
}
