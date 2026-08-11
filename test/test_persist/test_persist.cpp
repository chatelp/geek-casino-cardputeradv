// Sauvegarde : aller-retour fidèle, corruption rejetée en bloc, joueurs et
// classement inclus.
#include <cstring>
#include <unity.h>

#include "persist.h"
#include "players.h"

void setUp() {}
void tearDown() {}

static core::Roster sampleRoster() {
    core::Roster r;
    core::addOrSwitchPlayer(r, "PIERRE");
    r.players[0].credits = 2450;
    r.players[0].spins = 321;
    r.players[0].bestWin = 500;
    core::addOrSwitchPlayer(r, "ZOE");
    r.players[1].credits = 80;
    core::addOrSwitchPlayer(r, "PIERRE");  // re-bascule sur le premier
    return r;
}

static void test_roundtrip_preserves_everything() {
    core::Roster r = sampleRoster();
    core::Settings st{3, 1, 1};

    const core::SaveData s = core::makeSave(r, st);
    TEST_ASSERT_TRUE(core::saveValid(s));

    core::Roster r2;
    core::Settings st2;
    TEST_ASSERT_TRUE(core::applySave(s, r2, st2));
    TEST_ASSERT_EQUAL_UINT8(2, r2.count);
    TEST_ASSERT_EQUAL_UINT8(0, r2.current);
    TEST_ASSERT_EQUAL_STRING("PIERRE", r2.players[0].name);
    TEST_ASSERT_EQUAL_INT32(2450, r2.players[0].credits);
    TEST_ASSERT_EQUAL_UINT32(321, r2.players[0].spins);
    TEST_ASSERT_EQUAL_UINT32(500, r2.players[0].bestWin);
    TEST_ASSERT_EQUAL_STRING("ZOE", r2.players[1].name);
    TEST_ASSERT_EQUAL_UINT8(3, st2.volume);
    TEST_ASSERT_EQUAL_UINT8(1, st2.muted);
    TEST_ASSERT_EQUAL_UINT8(1, st2.slotSkin);
}

static void test_any_flipped_byte_is_rejected() {
    const core::SaveData good = core::makeSave(sampleRoster(), core::Settings{});
    // Chaque octet corrompu, un par un : la somme de contrôle doit tout voir.
    for (size_t i = 0; i < sizeof(core::SaveData); ++i) {
        core::SaveData bad = good;
        reinterpret_cast<uint8_t*>(&bad)[i] ^= 0x5A;
        core::Roster r;
        core::Settings st;
        if (core::applySave(bad, r, st)) {
            // Seul octet libre : le padding, hors somme s'il est après sum ?
            // Non — tout octet avant `sum` est couvert, et corrompre `sum`
            // invalide la comparaison. Aucun octet ne doit passer.
            TEST_FAIL_MESSAGE("un octet corrompu a été accepté");
        }
    }
}

static void test_out_of_range_fields_are_rejected() {
    core::SaveData s = core::makeSave(sampleRoster(), core::Settings{});
    s.playerCount = core::kMaxPlayers + 1;
    s.sum = core::saveChecksum(s);  // même « bien signée », elle est absurde
    TEST_ASSERT_FALSE(core::saveValid(s));

    s = core::makeSave(sampleRoster(), core::Settings{});
    s.players[0].credits = -5;
    s.sum = core::saveChecksum(s);
    TEST_ASSERT_FALSE(core::saveValid(s));

    s = core::makeSave(sampleRoster(), core::Settings{});
    s.settings.volume = 9;
    s.sum = core::saveChecksum(s);
    TEST_ASSERT_FALSE(core::saveValid(s));
}

static void test_garbage_is_rejected() {
    core::SaveData s;
    std::memset(&s, 0xFF, sizeof(s));
    TEST_ASSERT_FALSE(core::saveValid(s));
    std::memset(&s, 0x00, sizeof(s));
    TEST_ASSERT_FALSE(core::saveValid(s));
}

int main() {
    UNITY_BEGIN();
    RUN_TEST(test_roundtrip_preserves_everything);
    RUN_TEST(test_any_flipped_byte_is_rejected);
    RUN_TEST(test_out_of_range_fields_are_rejected);
    RUN_TEST(test_garbage_is_rejected);
    return UNITY_END();
}
