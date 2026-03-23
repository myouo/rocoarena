// tests/entity/pet_damage_test.cpp
// Tests for Pet damage taking, HP invariants, and damage modifiers

#include <gtest/gtest.h>
#include <entity/Pet.h>
#include <entity/Species.h>

namespace {

Species makeSpecies(BS bs) {
    return Species(1, "TestPet", {AttrType::Fire, AttrType::None}, bs);
}

const IVData kMaxIVs{31, 31, 31, 31, 31, 31};
const EVData kZeroEVs{0, 0, 0, 0, 0, 0};

// Create a Pet at level 100 with Hardy nature, fully initialized
Pet makePet(Species& sp) {
    Pet pet(&sp, kMaxIVs, kZeroEVs);
    pet.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Hardy, 100);
    return pet;
}

} // namespace

// =============================================================================
// HP Invariants
// =============================================================================

TEST(PetDamage, HP_Never_Below_Zero) {
    auto sp = makeSpecies(BS{50, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    // Deal absurdly more damage than max HP
    pet.takeDamage(maxHP * 10);

    EXPECT_EQ(pet.currentHP(), 0);
    EXPECT_TRUE(pet.isFainted());
}

TEST(PetDamage, HP_Never_Exceeds_MaxHP) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    // Take some damage then try to over-heal
    pet.takeDamage(50);
    pet.restoreHP(maxHP * 2);

    EXPECT_EQ(pet.currentHP(), maxHP);
}

TEST(PetDamage, TakeDamage_Returns_Remaining_HP) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    int remaining = pet.takeDamage(100);
    EXPECT_EQ(remaining, maxHP - 100);
    EXPECT_EQ(pet.currentHP(), maxHP - 100);
}

TEST(PetDamage, TakeDamage_Zero_NoChange) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    pet.takeDamage(0);
    EXPECT_EQ(pet.currentHP(), maxHP);
}

TEST(PetDamage, TakeDamage_Negative_NoChange) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    pet.takeDamage(-50);
    EXPECT_EQ(pet.currentHP(), maxHP);
}

TEST(PetDamage, Fainted_After_Lethal_Damage) {
    auto sp = makeSpecies(BS{50, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);

    EXPECT_FALSE(pet.isFainted());
    pet.takeDamage(pet.maxHP());
    EXPECT_TRUE(pet.isFainted());
    EXPECT_EQ(pet.currentHP(), 0);
}

// =============================================================================
// Damage Modifiers
// =============================================================================

TEST(PetDamage, DamageImmunity_Blocks_All_Damage) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    pet.setDamageImmunityTurns(3);
    pet.takeDamage(9999);

    EXPECT_EQ(pet.currentHP(), maxHP);  // Immune, no damage
}

TEST(PetDamage, FlatDamageReduction_Reduces_Damage) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    pet.setFlatDamageReduction(30);
    pet.takeDamage(100);

    EXPECT_EQ(pet.currentHP(), maxHP - 70);
}

TEST(PetDamage, FlatDamageReduction_Cannot_Make_Damage_Negative) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    pet.setFlatDamageReduction(200);
    pet.takeDamage(50);

    EXPECT_EQ(pet.currentHP(), maxHP);  // Reduced to 0 damage, not healed
}

TEST(PetDamage, DamageMultiplier_Scales_Damage) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    pet.setDamageMultiplier(0.5);
    pet.takeDamage(100);

    // 100 * 0.5 = 50
    EXPECT_EQ(pet.currentHP(), maxHP - 50);
}

TEST(PetDamage, PerHitDamageReduction_Expires_After_Turns) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();

    pet.setPerHitDamageReduction(20, 1); // 20 reduction for 1 turn
    pet.takeDamage(100); // Should get 100 - 20 = 80

    EXPECT_EQ(pet.currentHP(), maxHP - 80);

    pet.tickDamageReductionTurn(); // Turn 1 expires

    int hpBefore = pet.currentHP();
    pet.takeDamage(100); // No reduction anymore
    EXPECT_EQ(pet.currentHP(), hpBefore - 100);
}

// =============================================================================
// Restore HP
// =============================================================================

TEST(PetDamage, RestoreHP_Zero_NoChange) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);

    pet.takeDamage(50);
    int before = pet.currentHP();
    pet.restoreHP(0);
    EXPECT_EQ(pet.currentHP(), before);
}

TEST(PetDamage, RestoreHP_Negative_NoChange) {
    auto sp = makeSpecies(BS{100, 50, 50, 50, 50, 50});
    Pet pet = makePet(sp);

    pet.takeDamage(50);
    int before = pet.currentHP();
    pet.restoreHP(-10);
    EXPECT_EQ(pet.currentHP(), before);
}
