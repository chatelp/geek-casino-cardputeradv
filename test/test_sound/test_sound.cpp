// La contrainte matérielle devient un test : le HP ne restitue rien sous
// ~400 Hz, la composition tient entre 800 et 2600 Hz. Chaque note de chaque
// motif est vérifiée — une note grave serait simplement inaudible.
#include <unity.h>

#include "sound.h"

void setUp() {}
void tearDown() {}

static const core::Cue kAllCues[] = {
    core::Cue::SpinStart, core::Cue::ReelStop1, core::Cue::ReelStop2,
    core::Cue::ReelStop3, core::Cue::WinSmall, core::Cue::WinMid,
    core::Cue::WinBig, core::Cue::Jackpot, core::Cue::Bailout,
    core::Cue::BetChange,
};

static void test_every_note_is_audible_on_this_speaker() {
    for (const core::Cue c : kAllCues) {
        const core::Cadence cd = core::cadenceOf(c);
        TEST_ASSERT_TRUE(cd.count > 0);
        for (uint8_t i = 0; i < cd.count; ++i) {
            TEST_ASSERT_TRUE_MESSAGE(cd.tones[i].hz >= core::kAudibleMinHz,
                                     "note sous 800 Hz : muette sur ce HP");
            TEST_ASSERT_TRUE_MESSAGE(cd.tones[i].hz <= core::kAudibleMaxHz,
                                     "note au-dessus de 2600 Hz");
        }
    }
}

static void test_no_cadence_overflows_the_pcm_buffer() {
    for (const core::Cue c : kAllCues) {
        TEST_ASSERT_TRUE_MESSAGE(core::cadenceMs(c) <= core::kMaxCueMs,
                                 "motif plus long que le tampon PCM");
    }
}

static void test_reel_stops_ascend() {
    // La cascade s'entend : chaque arrêt est plus aigu que le précédent.
    const uint16_t a = core::cadenceOf(core::reelStopCue(0)).tones[0].hz;
    const uint16_t b = core::cadenceOf(core::reelStopCue(1)).tones[0].hz;
    const uint16_t c = core::cadenceOf(core::reelStopCue(2)).tones[0].hz;
    TEST_ASSERT_TRUE(a < b);
    TEST_ASSERT_TRUE(b < c);
}

static void test_win_sounds_escalate_in_length() {
    TEST_ASSERT_TRUE(core::cadenceMs(core::Cue::WinSmall) <
                     core::cadenceMs(core::Cue::WinMid));
    TEST_ASSERT_TRUE(core::cadenceMs(core::Cue::WinMid) <
                     core::cadenceMs(core::Cue::WinBig));
    TEST_ASSERT_TRUE(core::cadenceMs(core::Cue::WinBig) <
                     core::cadenceMs(core::Cue::Jackpot));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_every_note_is_audible_on_this_speaker);
    RUN_TEST(test_no_cadence_overflows_the_pcm_buffer);
    RUN_TEST(test_reel_stops_ascend);
    RUN_TEST(test_win_sounds_escalate_in_length);
    return UNITY_END();
}
