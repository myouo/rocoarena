// tests/battle/buff_test.cpp
// Tests for Buff stat stage mechanics and invariants

#include <gtest/gtest.h>
#include <battle/Buff.h>
#include <entity/Pet.h>
#include <entity/Species.h>

namespace {

Species makeSpecies() {
    return Species(1, "TestPet", {AttrType::Fire, AttrType::None}, BS{100, 100, 100, 100, 100, 100});
}

Pet makePet(Species& sp) {
    Pet pet(&sp, IVData{31,31,31,31,31,31}, EVData{0,0,0,0,0,0});
    pet.calcRealStat(sp.baseStats(), IVData{31,31,31,31,31,31}, EVData{0,0,0,0,0,0}, NatureType::Hardy, 100);
    return pet;
}

} // namespace

// =============================================================================
// Stat Stage Invariants: stages clamped to [-6, +6]
// =============================================================================

TEST(BuffStages, Stage_Clamped_At_Plus6) {
    Buff buff;
    // Apply +12 stages, should cap at +6
    buff.changeStage(Stat::Atk, 12);
    EXPECT_EQ(buff.stage(Stat::Atk), 6);
}

TEST(BuffStages, Stage_Clamped_At_Minus6) {
    Buff buff;
    buff.changeStage(Stat::Atk, -12);
    EXPECT_EQ(buff.stage(Stat::Atk), -6);
}

TEST(BuffStages, Stage_Incremental_Changes) {
    Buff buff;
    buff.changeStage(Stat::Def, 2);
    EXPECT_EQ(buff.stage(Stat::Def), 2);

    buff.changeStage(Stat::Def, 3);
    EXPECT_EQ(buff.stage(Stat::Def), 5);

    buff.changeStage(Stat::Def, 3); // Would be 8, capped at 6
    EXPECT_EQ(buff.stage(Stat::Def), 6);
}

TEST(BuffStages, Stage_Negative_Then_Positive) {
    Buff buff;
    buff.changeStage(Stat::SpA, -3);
    EXPECT_EQ(buff.stage(Stat::SpA), -3);

    buff.changeStage(Stat::SpA, 5);
    EXPECT_EQ(buff.stage(Stat::SpA), 2);
}

TEST(BuffStages, ResetStages_All_Zero) {
    Buff buff;
    buff.changeStage(Stat::Atk, 4);
    buff.changeStage(Stat::Def, -3);
    buff.changeStage(Stat::Spe, 6);
    buff.resetStages();

    EXPECT_EQ(buff.stage(Stat::Atk), 0);
    EXPECT_EQ(buff.stage(Stat::Def), 0);
    EXPECT_EQ(buff.stage(Stat::Spe), 0);
}

// =============================================================================
// Stat Stage Application: multiplier on base stat
// Stage +1 = 3/2, +2 = 4/2, ..., +6 = 8/2
// Stage -1 = 2/3, -2 = 2/4, ..., -6 = 2/8
// =============================================================================

TEST(BuffStages, ApplyStage_Plus2_DoublesBaseStat) {
    Buff buff;
    buff.changeStage(Stat::Atk, 2);

    // At +2: multiplier = 4/2 = 2.0
    int modified = buff.applyStageToStat(Stat::Atk, 200);
    EXPECT_EQ(modified, 400);
}

TEST(BuffStages, ApplyStage_Minus1_ReducesStat) {
    Buff buff;
    buff.changeStage(Stat::Def, -1);

    // At -1: multiplier = 2/3
    int modified = buff.applyStageToStat(Stat::Def, 300);
    EXPECT_EQ(modified, 200);
}

TEST(BuffStages, ApplyStage_Zero_NoChange) {
    Buff buff;
    int modified = buff.applyStageToStat(Stat::Spe, 250);
    EXPECT_EQ(modified, 250);
}

// =============================================================================
// Ailment basics
// =============================================================================

TEST(BuffAilment, InitialState_NoAilments) {
    Buff buff;
    EXPECT_EQ(buff.primaryAilment(), Ailment::None);
    EXPECT_EQ(buff.secondaryAilment(), Ailment::None);
    EXPECT_FALSE(buff.isTrapped());
}

TEST(BuffAilment, ApplyBurn_SetsPrimary) {
    Buff buff;
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};
    bool applied = buff.applyAilment(Ailment::Burn, attrs);
    EXPECT_TRUE(applied);
    EXPECT_EQ(buff.primaryAilment(), Ailment::Burn);
}

TEST(BuffAilment, FireType_Immune_To_Burn) {
    Buff buff;
    std::array<AttrType, 2> attrs = {AttrType::Fire, AttrType::None};
    bool applied = buff.applyAilment(Ailment::Burn, attrs);
    EXPECT_FALSE(applied);
    EXPECT_EQ(buff.primaryAilment(), Ailment::None);
}

TEST(BuffAilment, ApplyPoison_SetsSecondary) {
    Buff buff;
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};
    bool applied = buff.applyAilment(Ailment::Poison, attrs);
    EXPECT_TRUE(applied);
    EXPECT_EQ(buff.secondaryAilment(), Ailment::Poison);
}

TEST(BuffAilment, ClearAilments_ClearsAll) {
    Buff buff;
    std::array<AttrType, 2> attrs = {AttrType::Normal, AttrType::None};
    buff.applyAilment(Ailment::Burn, attrs);
    buff.applyAilment(Ailment::Poison, attrs);
    buff.clearAilments();

    EXPECT_EQ(buff.primaryAilment(), Ailment::None);
    EXPECT_EQ(buff.secondaryAilment(), Ailment::None);
}

TEST(BuffAilment, DoubleLoss_DefaultOff) {
    Buff buff;
    EXPECT_FALSE(buff.hasDoubleLoss());
}

TEST(BuffAilment, StageLocked_Prevents_StageChanges) {
    Buff buff;
    buff.setStageLocked(true);
    EXPECT_TRUE(buff.stageLocked());
    
    // With stage locked, changes should be rejected (returns 0)
    int delta = buff.changeStage(Stat::Atk, 3);
    EXPECT_EQ(delta, 0);
    EXPECT_EQ(buff.stage(Stat::Atk), 0);
}
