// Tests de lib/core/rng — déterminisme, bornes, absence de biais grossier.
#include <unity.h>

#include "rng.h"

void setUp() {}
void tearDown() {}

static void test_xorshift_is_deterministic() {
    core::seedXorShift(1234);
    const uint32_t a1 = core::xorShift32();
    const uint32_t a2 = core::xorShift32();
    core::seedXorShift(1234);
    TEST_ASSERT_EQUAL_UINT32(a1, core::xorShift32());
    TEST_ASSERT_EQUAL_UINT32(a2, core::xorShift32());
}

static void test_xorshift_seed_changes_sequence() {
    core::seedXorShift(1);
    const uint32_t a = core::xorShift32();
    core::seedXorShift(2);
    TEST_ASSERT_NOT_EQUAL(a, core::xorShift32());
}

static void test_draw_below_degenerate_cases() {
    core::seedXorShift(42);
    TEST_ASSERT_EQUAL_UINT32(0, core::drawBelow(core::xorShift32, 0));
    TEST_ASSERT_EQUAL_UINT32(0, core::drawBelow(core::xorShift32, 1));
}

static void test_draw_below_stays_in_range() {
    core::seedXorShift(42);
    const uint32_t ns[] = {2, 3, 6, 10, 97, 1024};
    for (uint32_t n : ns) {
        for (int i = 0; i < 10000; ++i) {
            TEST_ASSERT_LESS_THAN_UINT32(n, core::drawBelow(core::xorShift32, n));
        }
    }
}

static void test_draw_below_roughly_uniform() {
    // Graine fixe → résultat stable : pas un test statistique flottant.
    core::seedXorShift(0xCA51704D);
    constexpr uint32_t n = 6;
    constexpr int draws = 60000;
    int counts[n] = {0};
    for (int i = 0; i < draws; ++i) {
        counts[core::drawBelow(core::xorShift32, n)]++;
    }
    const int expected = draws / static_cast<int>(n);  // 10000
    for (uint32_t k = 0; k < n; ++k) {
        TEST_ASSERT_INT_WITHIN(300, expected, counts[k]);  // ±3 %
    }
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_xorshift_is_deterministic);
    RUN_TEST(test_xorshift_seed_changes_sequence);
    RUN_TEST(test_draw_below_degenerate_cases);
    RUN_TEST(test_draw_below_stays_in_range);
    RUN_TEST(test_draw_below_roughly_uniform);
    return UNITY_END();
}
