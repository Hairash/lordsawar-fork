// Deterministic Combat System Unit Tests
//
// These tests verify the combat damage calculation using the SAME code
// as the actual game (combat-damage.h).
//
// Compile and run:
//   cd src
//   g++ -std=c++11 -o fight-test fight-test.cpp
//   ./fight-test

#include <iostream>
#include <cmath>
#include <string>

// Include the REAL damage calculation - same code used by Fight class
#include "combat-damage.h"

// Test framework macros
#define TEST(name) void test_##name()
#define RUN_TEST(name) do { \
    std::cout << "Running " << #name << "... "; \
    try { \
        test_##name(); \
        std::cout << "PASSED" << std::endl; \
        passed++; \
    } catch (const std::exception& e) { \
        std::cout << "FAILED: " << e.what() << std::endl; \
        failed++; \
    } \
} while(0)

#define ASSERT_EQ(expected, actual) do { \
    if ((expected) != (actual)) { \
        throw std::runtime_error("Expected " + std::to_string(expected) + " but got " + std::to_string(actual)); \
    } \
} while(0)

#define ASSERT_NEAR(expected, actual, epsilon) do { \
    if (std::abs((expected) - (actual)) > (epsilon)) { \
        throw std::runtime_error("Expected " + std::to_string(expected) + " +/- " + std::to_string(epsilon) + " but got " + std::to_string(actual)); \
    } \
} while(0)

#define ASSERT_TRUE(condition) do { \
    if (!(condition)) { \
        throw std::runtime_error("Assertion failed: " #condition); \
    } \
} while(0)

#define ASSERT_FALSE(condition) do { \
    if (condition) { \
        throw std::runtime_error("Assertion failed: expected false for " #condition); \
    } \
} while(0)

// Combat simulation using the real calculateCombatDamage function
struct CombatResult {
    bool attacker_won;
    double attacker_final_hp;
    double defender_final_hp;
    int total_rounds;
};

CombatResult simulateCombat(int att_str, double att_hp, int def_str, double def_hp, bool intense = false) {
    CombatResult result;
    result.total_rounds = 0;
    bool attacker_turn = true;

    while (att_hp > 0 && def_hp > 0) {
        result.total_rounds++;

        if (attacker_turn) {
            // Uses the REAL damage calculation from combat-damage.h
            double damage = calculateCombatDamage(att_str, def_str, intense);
            def_hp -= damage;
        } else {
            double damage = calculateCombatDamage(def_str, att_str, intense);
            att_hp -= damage;
        }

        attacker_turn = !attacker_turn;
    }

    result.attacker_won = def_hp <= 0;
    result.attacker_final_hp = att_hp;
    result.defender_final_hp = def_hp;
    return result;
}

// Global counters
int passed = 0;
int failed = 0;

//=============================================================================
// TEST 1: Single hit damage calculation
//=============================================================================
TEST(single_hit_damage_calculation) {
    // Attacker str 5 vs Defender str 2 (normal combat, d20)
    // Expected: (5/20) * ((20-2)/20) * 5 = 0.25 * 0.9 * 5 = 1.125
    double damage = calculateCombatDamage(5, 2, false);
    ASSERT_NEAR(1.125, damage, 0.001);

    // Defender damage: (2/20) * ((20-5)/20) * 5 = 0.1 * 0.75 * 5 = 0.375
    double defender_damage = calculateCombatDamage(2, 5, false);
    ASSERT_NEAR(0.375, defender_damage, 0.001);
}

//=============================================================================
// TEST 2: Combat ends when defender dies mid-round
//=============================================================================
TEST(combat_ends_when_defender_dies) {
    // A1 (str 5, HP 2) vs D1 (str 2, HP 1)
    // Attacker damage = 1.125, so defender dies in first hit
    CombatResult result = simulateCombat(5, 2.0, 2, 1.0);

    ASSERT_TRUE(result.attacker_won);
    ASSERT_TRUE(result.defender_final_hp <= 0);
    ASSERT_NEAR(2.0, result.attacker_final_hp, 0.001); // Attacker took no damage
    ASSERT_EQ(1, result.total_rounds); // Only 1 round needed
}

//=============================================================================
// TEST 3: Defender replacement on death (simulated with stack)
//=============================================================================
TEST(defender_replacement_on_death) {
    // Simulate: A1 (str 5, HP 5) vs D1 (str 2, HP 1), D2 (str 3, HP 2)
    // D1 dies after first hit, D2 takes over

    double att_hp = 5.0;
    int att_str = 5;

    // Defenders queue
    double def1_hp = 1.0;
    int def1_str = 2;
    double def2_hp = 2.0;
    int def2_str = 3;

    int rounds = 0;
    bool attacker_turn = true;
    int current_defender = 1;
    double current_def_hp = def1_hp;
    int current_def_str = def1_str;
    bool all_defenders_dead = false;

    while (att_hp > 0 && !all_defenders_dead) {
        rounds++;

        if (attacker_turn) {
            double damage = calculateCombatDamage(att_str, current_def_str, false);
            current_def_hp -= damage;

            // Check if defender died
            if (current_def_hp <= 0) {
                if (current_defender == 1) {
                    // Replace with D2
                    current_defender = 2;
                    current_def_hp = def2_hp;
                    current_def_str = def2_str;
                } else {
                    // No more defenders
                    all_defenders_dead = true;
                }
            }
        } else {
            double damage = calculateCombatDamage(current_def_str, att_str, false);
            att_hp -= damage;
        }

        attacker_turn = !attacker_turn;

        if (rounds > 100) break; // Safety limit
    }

    ASSERT_TRUE(all_defenders_dead); // All defenders dead
    ASSERT_TRUE(att_hp > 0); // Attacker survived
    ASSERT_EQ(2, current_defender); // D2 was engaged
}

//=============================================================================
// TEST 4: Attacker replacement on death
//=============================================================================
TEST(attacker_replacement_on_death) {
    // A1 (str 2, HP 1), A2 (str 4, HP 3) vs D1 (str 5, HP 5)
    // A1 should die and A2 takes over

    double att1_hp = 1.0;
    int att1_str = 2;
    double att2_hp = 3.0;
    int att2_str = 4;

    double def_hp = 5.0;
    int def_str = 5;

    int rounds = 0;
    bool attacker_turn = true;
    int current_attacker = 1;
    double current_att_hp = att1_hp;
    int current_att_str = att1_str;

    while ((current_attacker <= 2 || current_att_hp > 0) && def_hp > 0) {
        rounds++;

        if (attacker_turn) {
            double damage = calculateCombatDamage(current_att_str, def_str, false);
            def_hp -= damage;
        } else {
            double damage = calculateCombatDamage(def_str, current_att_str, false);
            current_att_hp -= damage;

            // Check if attacker died
            if (current_att_hp <= 0 && current_attacker == 1) {
                // Replace with A2
                current_attacker = 2;
                current_att_hp = att2_hp;
                current_att_str = att2_str;
            }
        }

        attacker_turn = !attacker_turn;

        if (rounds > 100) break; // Safety limit
    }

    ASSERT_EQ(2, current_attacker); // A2 was engaged (A1 died)
}

//=============================================================================
// TEST 5: Equal strength combat
//=============================================================================
TEST(equal_strength_combat) {
    // Both str 5, HP 2
    // Equal damage: (5/20) * (15/20) * 5 = 0.25 * 0.75 * 5 = 0.9375
    double damage = calculateCombatDamage(5, 5, false);
    ASSERT_NEAR(0.9375, damage, 0.001);

    // Combat: attacker goes first, so attacker has advantage
    CombatResult result = simulateCombat(5, 2.0, 5, 2.0);

    // Both deal equal damage, but attacker hits first
    ASSERT_TRUE(result.attacker_won);
}

//=============================================================================
// TEST 6: Minimum damage threshold
//=============================================================================
TEST(minimum_damage_threshold) {
    // str 1 attacker vs str 9 defender
    // Calculated: (1/20) * (11/20) * 5 = 0.05 * 0.55 * 5 = 0.1375
    double damage1 = calculateCombatDamage(1, 9, false);
    ASSERT_NEAR(0.1375, damage1, 0.001);

    // str 1 vs str 1
    // (1/20) * (19/20) * 5 = 0.05 * 0.95 * 5 = 0.2375
    double damage2 = calculateCombatDamage(1, 1, false);
    ASSERT_NEAR(0.2375, damage2, 0.001);

    // Verify minimum is enforced
    ASSERT_TRUE(damage1 >= MINIMUM_DAMAGE_PER_HIT);
    ASSERT_TRUE(damage2 >= MINIMUM_DAMAGE_PER_HIT);
}

//=============================================================================
// TEST 7: Intense combat mode (24-sided dice)
//=============================================================================
TEST(intense_combat_mode) {
    // Same str 5 vs str 2, but with d24
    // Normal: (5/20) * (18/20) * 5 = 1.125
    // Intense: (5/24) * (22/24) * 5 = 0.2083 * 0.9167 * 5 = 0.955
    double normal_damage = calculateCombatDamage(5, 2, false);
    double intense_damage = calculateCombatDamage(5, 2, true);

    ASSERT_NEAR(1.125, normal_damage, 0.001);
    ASSERT_NEAR(0.9548, intense_damage, 0.001);

    // Intense combat should deal less damage (harder to hit)
    ASSERT_TRUE(intense_damage < normal_damage);
}

//=============================================================================
// TEST 8: Combat resolves correctly with multiple rounds
//=============================================================================
TEST(multiple_round_combat) {
    // str 5 vs str 2, both HP 2
    // Attacker damage: 1.125, Defender damage: 0.375
    // Round 1: A hits D -> D HP = 2 - 1.125 = 0.875
    // Round 2: D hits A -> A HP = 2 - 0.375 = 1.625
    // Round 3: A hits D -> D HP = 0.875 - 1.125 = -0.25 (dead)

    CombatResult result = simulateCombat(5, 2.0, 2, 2.0);

    ASSERT_TRUE(result.attacker_won);
    ASSERT_NEAR(1.625, result.attacker_final_hp, 0.001);
    ASSERT_TRUE(result.defender_final_hp <= 0);
    ASSERT_EQ(3, result.total_rounds);
}

//=============================================================================
// TEST 9: Deterministic - same input always produces same output
//=============================================================================
TEST(deterministic_results) {
    // Run the same combat multiple times, should always get identical results
    CombatResult result1 = simulateCombat(5, 2.0, 2, 2.0);
    CombatResult result2 = simulateCombat(5, 2.0, 2, 2.0);
    CombatResult result3 = simulateCombat(5, 2.0, 2, 2.0);

    ASSERT_EQ(result1.attacker_won, result2.attacker_won);
    ASSERT_EQ(result2.attacker_won, result3.attacker_won);
    ASSERT_NEAR(result1.attacker_final_hp, result2.attacker_final_hp, 0.0001);
    ASSERT_NEAR(result2.attacker_final_hp, result3.attacker_final_hp, 0.0001);
    ASSERT_EQ(result1.total_rounds, result2.total_rounds);
    ASSERT_EQ(result2.total_rounds, result3.total_rounds);
}

//=============================================================================
// TEST 10: Verify constants match expected values
//=============================================================================
TEST(constants_verification) {
    // Verify the constants from combat-damage.h are correct
    ASSERT_EQ(20u, BATTLE_DICE_SIDES_NORMAL);
    ASSERT_EQ(24u, BATTLE_DICE_SIDES_INTENSE);
    ASSERT_NEAR(5.0, DETERMINISTIC_DAMAGE_MULTIPLIER, 0.001);
    ASSERT_NEAR(0.1, MINIMUM_DAMAGE_PER_HIT, 0.001);
}

//=============================================================================
// MAIN
//=============================================================================
int main() {
    std::cout << "========================================" << std::endl;
    std::cout << "Deterministic Combat System Unit Tests" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Testing: combat-damage.h (real implementation)" << std::endl;
    std::cout << std::endl;

    RUN_TEST(single_hit_damage_calculation);
    RUN_TEST(combat_ends_when_defender_dies);
    RUN_TEST(defender_replacement_on_death);
    RUN_TEST(attacker_replacement_on_death);
    RUN_TEST(equal_strength_combat);
    RUN_TEST(minimum_damage_threshold);
    RUN_TEST(intense_combat_mode);
    RUN_TEST(multiple_round_combat);
    RUN_TEST(deterministic_results);
    RUN_TEST(constants_verification);

    std::cout << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Results: " << passed << " passed, " << failed << " failed" << std::endl;
    std::cout << "========================================" << std::endl;

    return failed > 0 ? 1 : 0;
}
