// tests/entity/attr_chart_test.cpp
// Tests for type effectiveness (AttrChart) and attribute advantages

#include <gtest/gtest.h>
#include <entity/Attr.h>

// =============================================================================
// Basic Type Matchups
// =============================================================================

TEST(AttrChart, Fire_SuperEffective_Against_Grass) {
    double adv = AttrChart::getAttrAdvantage(AttrType::Fire, AttrType::Grass);
    EXPECT_DOUBLE_EQ(adv, 2.0);
}

TEST(AttrChart, Water_SuperEffective_Against_Fire) {
    double adv = AttrChart::getAttrAdvantage(AttrType::Water, AttrType::Fire);
    EXPECT_DOUBLE_EQ(adv, 2.0);
}

TEST(AttrChart, Grass_SuperEffective_Against_Water) {
    double adv = AttrChart::getAttrAdvantage(AttrType::Grass, AttrType::Water);
    EXPECT_DOUBLE_EQ(adv, 2.0);
}

TEST(AttrChart, Fire_Resisted_By_Water) {
    double adv = AttrChart::getAttrAdvantage(AttrType::Fire, AttrType::Water);
    EXPECT_DOUBLE_EQ(adv, 0.5);
}

TEST(AttrChart, Normal_Neutral_Against_Normal) {
    double adv = AttrChart::getAttrAdvantage(AttrType::Normal, AttrType::Normal);
    EXPECT_DOUBLE_EQ(adv, 1.0);
}

TEST(AttrChart, Normal_Resisted_By_Rock) {
    double adv = AttrChart::getAttrAdvantage(AttrType::Normal, AttrType::Rock);
    EXPECT_DOUBLE_EQ(adv, 0.5);
}

TEST(AttrChart, None_Always_Neutral) {
    double adv = AttrChart::getAttrAdvantage(AttrType::Fire, AttrType::None);
    EXPECT_DOUBLE_EQ(adv, 1.0);
}

// =============================================================================
// Dual-type matchups
// Score +2 => 3.0x; +1 => 2.0x; 0 => 1.0x; -1 => 0.5x; -2 => 1/3x
// =============================================================================

TEST(AttrChart, DualType_BothWeak_Yields_3x) {
    // Ground vs (Fire, Electric) = Counter + Counter = score 2 => 3.0
    double adv = AttrChart::getAttrAdvantage(
        AttrType::Ground,
        std::array<AttrType, 2>{AttrType::Fire, AttrType::Electric});
    EXPECT_DOUBLE_EQ(adv, 3.0);
}

TEST(AttrChart, DualType_OneWeak_OneNeutral_Yields_2x) {
    // Fire vs (Grass, Normal) = Counter + Neutral = score 1 => 2.0
    double adv = AttrChart::getAttrAdvantage(
        AttrType::Fire,
        std::array<AttrType, 2>{AttrType::Grass, AttrType::Normal});
    EXPECT_DOUBLE_EQ(adv, 2.0);
}

TEST(AttrChart, DualType_WithNone_SingleType) {
    // Fire vs (Grass, None) should behave like single-type Grass
    double dual = AttrChart::getAttrAdvantage(
        AttrType::Fire,
        std::array<AttrType, 2>{AttrType::Grass, AttrType::None});
    double single = AttrChart::getAttrAdvantage(AttrType::Fire, AttrType::Grass);
    EXPECT_DOUBLE_EQ(dual, single);
}

// =============================================================================
// God (Divine) Type Tests
// =============================================================================

TEST(AttrChart, GodType_Against_NonGod_Is_2x) {
    // dFire vs Normal = 2.0 (god vs non-god always 2.0)
    double adv = AttrChart::getAttrAdvantage(AttrType::dFire, AttrType::Normal);
    EXPECT_DOUBLE_EQ(adv, 2.0);
}

TEST(AttrChart, GodType_Against_None_Is_Neutral) {
    double adv = AttrChart::getAttrAdvantage(AttrType::dFire, AttrType::None);
    EXPECT_DOUBLE_EQ(adv, 1.0);
}

TEST(AttrChart, IsGodAttr_Correct) {
    EXPECT_TRUE(AttrChart::isGodAttr(AttrType::dFire));
    EXPECT_TRUE(AttrChart::isGodAttr(AttrType::dGrass));
    EXPECT_TRUE(AttrChart::isGodAttr(AttrType::dWater));
    EXPECT_FALSE(AttrChart::isGodAttr(AttrType::Fire));
    EXPECT_FALSE(AttrChart::isGodAttr(AttrType::Normal));
    EXPECT_FALSE(AttrChart::isGodAttr(AttrType::None));
}
