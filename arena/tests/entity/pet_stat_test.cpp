// tests/entity/pet_stat_test.cpp
// Tests for Pet stat calculation formulas (IV, EV, Nature modifiers)

#include <gtest/gtest.h>
#include <entity/Pet.h>
#include <entity/Species.h>

namespace {

// Helper: create a Species with known base stats
Species makeSpecies(int id, std::string name, AttrType attr1, AttrType attr2, BS bs) {
    return Species(id, std::move(name), {attr1, attr2}, bs);
}

// Convenience: max IVs (31 across all stats)
const IVData kMaxIVs{31, 31, 31, 31, 31, 31};
const IVData kZeroIVs{0, 0, 0, 0, 0, 0};
const EVData kZeroEVs{0, 0, 0, 0, 0, 0};
const EVData kMaxAtkSpeEVs{0, 252, 0, 0, 0, 252};

} // namespace

// =============================================================================
// Stat Calculation: HP formula
//   HP = floor((Base*2 + IV) * Lv / 100) + Lv + 10 + (EV >> 2)
// =============================================================================

TEST(PetStatCalc, HP_Level100_MaxIV_ZeroEV) {
    // Species with 100 base HP
    auto sp = makeSpecies(1, "TestPet", AttrType::Fire, AttrType::None, BS{100, 80, 70, 80, 70, 90});
    Pet pet(&sp, kMaxIVs, kZeroEVs);
    pet.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Hardy, 100);

    // HP = floor((100*2+31)*100/100) + 100 + 10 + 0 = 231 + 110 = 341
    EXPECT_EQ(pet.maxHP(), 341);
    EXPECT_EQ(pet.currentHP(), 341);  // Full HP after calc
}

TEST(PetStatCalc, HP_Level100_ZeroIV_ZeroEV) {
    auto sp = makeSpecies(1, "TestPet", AttrType::Fire, AttrType::None, BS{100, 80, 70, 80, 70, 90});
    Pet pet(&sp, kZeroIVs, kZeroEVs);
    pet.calcRealStat(sp.baseStats(), kZeroIVs, kZeroEVs, NatureType::Hardy, 100);

    // HP = floor((100*2+0)*100/100) + 100 + 10 + 0 = 200 + 110 = 310
    EXPECT_EQ(pet.maxHP(), 310);
}

TEST(PetStatCalc, HP_WithEV_Level100) {
    auto sp = makeSpecies(1, "TestPet", AttrType::Fire, AttrType::None, BS{100, 80, 70, 80, 70, 90});
    EVData evs(252, 0, 0, 0, 0, 0); // 252 HP EVs
    Pet pet(&sp, kMaxIVs, evs);
    pet.calcRealStat(sp.baseStats(), kMaxIVs, evs, NatureType::Hardy, 100);

    // HP = floor((100*2+31)*100/100) + 100 + 10 + (252>>2) = 231 + 110 + 63 = 404
    EXPECT_EQ(pet.maxHP(), 404);
}

// =============================================================================
// Stat Calculation: Other stats
//   Stat = floor((Base*2 + IV) * Lv / 100) + 5 + (EV >> 2)
// =============================================================================

TEST(PetStatCalc, Attack_Level100_MaxIV_ZeroEV_Neutral) {
    auto sp = makeSpecies(1, "TestPet", AttrType::Fire, AttrType::None, BS{100, 80, 70, 80, 70, 90});
    Pet pet(&sp, kMaxIVs, kZeroEVs);
    pet.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Hardy, 100);

    // Atk = floor((80*2+31)*100/100) + 5 + 0 = 191 + 5 = 196
    EXPECT_EQ(pet.attack(), 196);
}

TEST(PetStatCalc, Attack_Level50_MaxIV_ZeroEV_Neutral) {
    auto sp = makeSpecies(1, "TestPet", AttrType::Fire, AttrType::None, BS{100, 80, 70, 80, 70, 90});
    Pet pet(&sp, kMaxIVs, kZeroEVs);
    pet.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Hardy, 50);

    // Atk = floor((80*2+31)*50/100) + 5 + 0 = floor(95.5) + 5 = 95 + 5 = 100
    EXPECT_EQ(pet.attack(), 100);
}

// =============================================================================
// Nature Modifier tests
// Adamant: +Atk, -SpA (1.1x Atk, 0.9x SpA)
// =============================================================================

TEST(PetStatCalc, Nature_Adamant_BoostsAtk_DropsSpA) {
    auto sp = makeSpecies(1, "TestPet", AttrType::Fire, AttrType::None, BS{100, 80, 70, 80, 70, 90});
    Pet petNeutral(&sp, kMaxIVs, kZeroEVs);
    petNeutral.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Hardy, 100);

    Pet petAdamant(&sp, kMaxIVs, kZeroEVs);
    petAdamant.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Adamant, 100);

    // Adamant: Atk boosted, SpA dropped. Truncation, not rounding.
    EXPECT_EQ(petAdamant.attack(), static_cast<int>(petNeutral.attack() * 1.1));
    EXPECT_EQ(petAdamant.specialAttack(), static_cast<int>(petNeutral.specialAttack() * 0.9));

    // Defense should remain unaffected
    EXPECT_EQ(petAdamant.defense(), petNeutral.defense());
}

TEST(PetStatCalc, Nature_Timid_BoostsSpe_DropsAtk) {
    auto sp = makeSpecies(1, "TestPet", AttrType::Fire, AttrType::None, BS{100, 80, 70, 80, 70, 90});
    Pet petNeutral(&sp, kMaxIVs, kZeroEVs);
    petNeutral.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Hardy, 100);

    Pet petTimid(&sp, kMaxIVs, kZeroEVs);
    petTimid.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Timid, 100);

    RS rsNeutral = petNeutral.getRS();
    RS rsTimid = petTimid.getRS();

    EXPECT_EQ(rsTimid.rSpe, static_cast<int>(rsNeutral.rSpe * 1.1));
    EXPECT_EQ(rsTimid.rAtk, static_cast<int>(rsNeutral.rAtk * 0.9));
}

TEST(PetStatCalc, Nature_Hardy_NoChanges) {
    auto sp = makeSpecies(1, "TestPet", AttrType::Fire, AttrType::None, BS{100, 100, 100, 100, 100, 100});
    Pet petHardy(&sp, kMaxIVs, kZeroEVs);
    petHardy.calcRealStat(sp.baseStats(), kMaxIVs, kZeroEVs, NatureType::Hardy, 100);

    // Hardy: up == down (Atk, Atk), so no nature modification applied
    // All non-HP stats should be: floor((100*2+31)*100/100) + 5 = 231 + 5 = 236
    EXPECT_EQ(petHardy.attack(), 236);
    EXPECT_EQ(petHardy.defense(), 236);
    EXPECT_EQ(petHardy.specialAttack(), 236);
    EXPECT_EQ(petHardy.specialDefense(), 236);
}
