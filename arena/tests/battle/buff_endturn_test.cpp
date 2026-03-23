// tests/battle/buff_endturn_test.cpp
// Strategy tests #8, #9: End-of-turn ailment damage settlement

#include <gtest/gtest.h>
#include <battle/Buff.h>
#include <entity/Pet.h>
#include <entity/Species.h>

namespace {

Species makeSpecies(BS bs) {
    return Species(1, "TestPet", {AttrType::Normal, AttrType::None}, bs);
}

Pet makePet(Species& sp) {
    IVData iv{31,31,31,31,31,31};
    EVData ev{0,0,0,0,0,0};
    Pet pet(&sp, iv, ev);
    pet.calcRealStat(sp.baseStats(), iv, ev, NatureType::Hardy, 100);
    return pet;
}

} // namespace

// =============================================================================
// #8: Settlement_Burn_Damage_Applied_End_Of_Turn
// Burn deals floor(maxHP * 0.125), capped at 200
// =============================================================================

TEST(BuffEndTurn, BurnDamage_AppliedEndOfTurn) {
    auto sp = makeSpecies(BS{100, 100, 100, 100, 100, 100});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};

    pet.buff().applyAilment(Ailment::Burn, attrs);
    EXPECT_EQ(pet.buff().primaryAilment(), Ailment::Burn);

    int hpBefore = pet.currentHP();
    pet.buff().onEndTurnNonControl(pet, nullptr);

    int expectedDmg = std::min(static_cast<int>(maxHP * 0.125), 200);
    EXPECT_EQ(pet.currentHP(), hpBefore - expectedDmg);
}

// =============================================================================
// #9: Settlement_Multiple_End_Of_Turn_Effects_Execute_In_Correct_Order
// Both Burn (primary) and Poison (secondary) apply damage
// =============================================================================

TEST(BuffEndTurn, MultipleEffects_BurnAndPoison) {
    auto sp = makeSpecies(BS{200, 100, 100, 100, 100, 100}); // High HP
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};

    pet.buff().applyAilment(Ailment::Burn, attrs);
    pet.buff().applyAilment(Ailment::Poison, attrs);
    EXPECT_EQ(pet.buff().primaryAilment(), Ailment::Burn);
    EXPECT_EQ(pet.buff().secondaryAilment(), Ailment::Poison);

    int hpBefore = pet.currentHP();
    pet.buff().onEndTurnNonControl(pet, nullptr);

    int burnDmg = std::min(static_cast<int>(maxHP * 0.125), 200);
    int poisonDmg = std::min(static_cast<int>(maxHP * 0.125), 200);
    int totalDmg = burnDmg + poisonDmg;

    EXPECT_EQ(pet.currentHP(), hpBefore - totalDmg);
}

// =============================================================================
// Poison damage: floor(maxHP * 0.125), capped at 200
// =============================================================================

TEST(BuffEndTurn, PoisonDamage_Applied) {
    auto sp = makeSpecies(BS{100, 100, 100, 100, 100, 100});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};

    pet.buff().applyAilment(Ailment::Poison, attrs);
    EXPECT_EQ(pet.buff().secondaryAilment(), Ailment::Poison);

    int hpBefore = pet.currentHP();
    pet.buff().onEndTurnNonControl(pet, nullptr);

    int expectedDmg = std::min(static_cast<int>(maxHP * 0.125), 200);
    EXPECT_EQ(pet.currentHP(), hpBefore - expectedDmg);
}

// =============================================================================
// Toxic damage: escalating floor(maxHP * 0.0625 * stacks), capped at 500
// =============================================================================

TEST(BuffEndTurn, ToxicDamage_Escalates) {
    auto sp = makeSpecies(BS{200, 100, 100, 100, 100, 100}); // High HP
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};

    pet.buff().applyAilment(Ailment::Toxic, attrs);
    EXPECT_EQ(pet.buff().secondaryAilment(), Ailment::Toxic);

    // Turn 1: stacks=1
    int hpBefore1 = pet.currentHP();
    pet.buff().onEndTurnNonControl(pet, nullptr);
    int dmg1 = hpBefore1 - pet.currentHP();
    int expected1 = std::min(static_cast<int>(maxHP * 0.0625 * 1), 500);
    EXPECT_EQ(dmg1, expected1);

    // Turn 2: stacks=2, damage doubles
    int hpBefore2 = pet.currentHP();
    pet.buff().onEndTurnNonControl(pet, nullptr);
    int dmg2 = hpBefore2 - pet.currentHP();
    int expected2 = std::min(static_cast<int>(maxHP * 0.0625 * 2), 500);
    EXPECT_EQ(dmg2, expected2);

    EXPECT_GT(dmg2, dmg1); // Toxic escalates
}

// =============================================================================
// Curse damage: maxHP / 4, capped at 500
// =============================================================================

TEST(BuffEndTurn, CurseDamage_Applied) {
    auto sp = makeSpecies(BS{200, 100, 100, 100, 100, 100});
    Pet pet = makePet(sp);
    int maxHP = pet.maxHP();
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};

    pet.buff().applyAilment(Ailment::Curse, attrs);
    EXPECT_EQ(pet.buff().secondaryAilment(), Ailment::Curse);

    int hpBefore = pet.currentHP();
    pet.buff().onEndTurnNonControl(pet, nullptr);

    int expectedDmg = std::min(maxHP / 4, 500);
    EXPECT_EQ(pet.currentHP(), hpBefore - expectedDmg);
}

// =============================================================================
// Parasite: drains floor(maxHP * 0.125) from self, heals opponent
// =============================================================================

TEST(BuffEndTurn, ParasiteDamage_DrainsAndHeals) {
    auto sp = makeSpecies(BS{100, 100, 100, 100, 100, 100});
    Pet self = makePet(sp);
    Pet opponent = makePet(sp);
    int maxHP = self.maxHP();
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};

    // Damage opponent first so healing is visible
    opponent.takeDamage(200);
    int oppHPBefore = opponent.currentHP();

    self.buff().applyAilment(Ailment::Parasite, attrs);
    int selfHPBefore = self.currentHP();
    self.buff().onEndTurnNonControl(self, &opponent);

    int drainDmg = static_cast<int>(maxHP * 0.125);
    EXPECT_EQ(self.currentHP(), selfHPBefore - drainDmg);
    EXPECT_EQ(opponent.currentHP(), oppHPBefore + drainDmg);
}

// =============================================================================
// No ailment: end-of-turn should not change HP
// =============================================================================

TEST(BuffEndTurn, NoAilment_NoDamage) {
    auto sp = makeSpecies(BS{100, 100, 100, 100, 100, 100});
    Pet pet = makePet(sp);
    int hpBefore = pet.currentHP();

    pet.buff().onEndTurnNonControl(pet, nullptr);

    EXPECT_EQ(pet.currentHP(), hpBefore);
}
