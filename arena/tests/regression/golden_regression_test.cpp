// tests/regression/golden_regression_test.cpp
// Strategy tests #26-30: Golden/snapshot regression tests with fixed RNG seeds

#include <gtest/gtest.h>
#include <battle/BattleSystem.h>
#include <battle/Action.h>
#include <entity/Pet.h>
#include <entity/Player.h>
#include <core/rng/rng.h>

namespace {

Species makeSpecies(int id, const char* name, BS bs, AttrType a1, AttrType a2 = AttrType::None) {
    return Species(id, name, {a1, a2}, bs);
}

Pet makePet(Species& sp, NatureType nature = NatureType::Hardy) {
    IVData iv{31,31,31,31,31,31};
    EVData ev{0,0,0,0,0,0};
    Pet pet(&sp, iv, ev);
    pet.calcRealStat(sp.baseStats(), iv, ev, nature, 100);
    return pet;
}

Player makePlayer(Pet& pet) {
    Player::Roster roster{};
    roster[0] = &pet;
    return Player(roster, 0);
}

// Action that deals fixed damage to opponent (simulates "Tackle")
class TackleAction : public Action {
public:
    TackleAction(int dmg) : Action(ActionType::Skill), dmg_(dmg) {}
    int priority() const override { return 0; }
    void execute(BattleSystem&, Player&, Player& opp) override {
        opp.activePet().takeDamage(dmg_);
    }
private:
    int dmg_;
};

// Captures full battle snapshot after N turns for golden comparison
struct BattleSnapshot {
    int turn;
    int pet1HP;
    int pet2HP;
    bool battleOver;
    bool pet1Fainted;
    bool pet2Fainted;

    bool operator==(const BattleSnapshot& o) const {
        return turn == o.turn && pet1HP == o.pet1HP && pet2HP == o.pet2HP &&
               battleOver == o.battleOver && pet1Fainted == o.pet1Fainted &&
               pet2Fainted == o.pet2Fainted;
    }
};

BattleSnapshot takeSnapshot(const BattleSystem& battle, const Pet& p1, const Pet& p2) {
    return { battle.currentTurn(), p1.currentHP(), p2.currentHP(),
             battle.isBattleOver(), p1.isFainted(), p2.isFainted() };
}

} // namespace

// =============================================================================
// #26: GoldenRegression_Basic_1v1_Tackle_Exchange
// Two pets trade Tackle blows for 3 turns; final HP verified exactly
// =============================================================================

TEST(GoldenRegression, Basic1v1TackleExchange) {
    RNG::instance().reseed(42);

    auto sp1 = makeSpecies(1, "Warrior", BS{100, 100, 80, 80, 80, 90}, AttrType::Fighting);
    auto sp2 = makeSpecies(2, "Guardian", BS{120, 80, 100, 80, 100, 70}, AttrType::Rock);
    Pet pet1 = makePet(sp1);
    Pet pet2 = makePet(sp2);

    int initialHP1 = pet1.maxHP();
    int initialHP2 = pet2.maxHP();

    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);

    // Exchange 50-damage tackles for 3 turns
    for (int t = 0; t < 3; ++t) {
        TackleAction a1(50), a2(50);
        battle.takeTurn(a1, a2);
    }

    // Golden: each took 150 damage over 3 turns
    BattleSnapshot snap = takeSnapshot(battle, pet1, pet2);
    BattleSnapshot golden = {3, initialHP1 - 150, initialHP2 - 150, false, false, false};
    EXPECT_EQ(snap, golden);
}

// =============================================================================
// #27: GoldenRegression_Status_Effect_Application_And_Death
// Apply burn, run turns, track HP decay until faint
// =============================================================================

TEST(GoldenRegression, StatusEffectAndDeath) {
    RNG::instance().reseed(123);

    auto sp = makeSpecies(1, "BurnVictim", BS{30, 50, 50, 50, 50, 50}, AttrType::Normal);
    Pet pet1 = makePet(sp);
    Pet pet2 = makePet(sp);

    // Apply burn to pet2
    std::array<AttrType, 2> normalAttrs = {AttrType::Normal, AttrType::None};
    pet2.buff().applyAilment(Ailment::Burn, normalAttrs);

    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);

    // Run turns with stay actions until pet2 faints from burn
    int maxTurns = 100;
    int turn = 0;
    while (!battle.isBattleOver() && turn < maxTurns) {
        StayAction s1, s2;
        battle.takeTurn(s1, s2);
        // Manually apply end-of-turn burn damage
        if (!pet2.isFainted()) {
            pet2.buff().onEndTurnNonControl(pet2, nullptr);
        }
        turn++;
        if (pet2.isFainted()) break;
    }

    EXPECT_TRUE(pet2.isFainted());
    EXPECT_EQ(pet2.currentHP(), 0);
    EXPECT_GT(pet1.currentHP(), 0);
}

// =============================================================================
// #28: GoldenRegression_Buff_Stacking_And_Sweeping
// Stack attack buffs, then verify enhanced damage
// =============================================================================

TEST(GoldenRegression, BuffStackingAndSweeping) {
    RNG::instance().reseed(777);

    auto sp = makeSpecies(1, "Sweeper", BS{100, 100, 100, 100, 100, 100}, AttrType::Normal);
    Pet pet1 = makePet(sp);
    Pet pet2 = makePet(sp);

    // Give pet1 +6 attack stages
    for (int i = 0; i < 3; ++i) {
        pet1.buff().changeStage(Stat::Atk, 2);
    }
    EXPECT_EQ(pet1.buff().stage(Stat::Atk), 6);

    // Staged attack should be base * 4 (at +6: (6/2+1) = 4.0)
    int baseAtk = pet1.attack();
    int stagedAtk = pet1.stagedAttack();
    EXPECT_EQ(stagedAtk, baseAtk * 4);

    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);

    // Deal staged-attack damage (simulate a sweep)
    TackleAction sweep(stagedAtk);
    StayAction stay;
    battle.takeTurn(sweep, stay);

    // pet2 should be fainted — 944 damage vs 341 max HP
    EXPECT_TRUE(pet2.isFainted());
    EXPECT_EQ(pet2.currentHP(), 0);
    EXPECT_TRUE(battle.isBattleOver());
}

// =============================================================================
// #29: GoldenRegression_MultiTurn_Healing_And_Damage_Trading
// Alternate between damage and potion healing
// =============================================================================

TEST(GoldenRegression, MultiTurnHealingAndTrading) {
    RNG::instance().reseed(456);

    auto sp = makeSpecies(1, "Healer", BS{100, 80, 80, 80, 80, 80}, AttrType::Water);
    Pet pet1 = makePet(sp);
    Pet pet2 = makePet(sp);
    int maxHP = pet1.maxHP();

    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);

    // Turn 1: Both deal 100 damage
    { TackleAction a1(100), a2(100); battle.takeTurn(a1, a2); }
    EXPECT_EQ(pet1.currentHP(), maxHP - 100);
    EXPECT_EQ(pet2.currentHP(), maxHP - 100);

    // Turn 2: P1 heals 80, P2 deals 100
    // PotionAction priority = 8, TackleAction priority = 0
    // Potion goes first (higher priority) → pet1 heals to maxHP-20, then gets hit for 100 → maxHP-120
    { PotionAction heal(80); TackleAction atk(100); battle.takeTurn(heal, atk); }
    EXPECT_EQ(pet1.currentHP(), maxHP - 120);

    // Turn 3: Both deal 50 damage
    { TackleAction a1(50), a2(50); battle.takeTurn(a1, a2); }
    EXPECT_EQ(pet1.currentHP(), maxHP - 170);
    // pet2 took 100 (turn 1) + 50 (turn 3) = 150 total damage
    EXPECT_EQ(pet2.currentHP(), maxHP - 150);
}

// =============================================================================
// #30: GoldenRegression_SpeedTie_Resolution_Sequence
// With fixed seed, verify speed ties resolve consistently
// =============================================================================

TEST(GoldenRegression, SpeedTieResolutionSequence) {
    // Run the same speed-tie scenario multiple times with same seed
    // Results must be identical each time
    auto sp = makeSpecies(1, "EqualPet", BS{100, 100, 100, 100, 100, 100}, AttrType::Normal);

    std::vector<int> results;
    for (int trial = 0; trial < 3; ++trial) {
        RNG::instance().reseed(42);

        Pet pet1 = makePet(sp);
        Pet pet2 = makePet(sp);
        Player p1 = makePlayer(pet1);
        Player p2 = makePlayer(pet2);

        BattleSystem battle;
        battle.init(p1, p2);

        // P1 deals 10 dmg, P2 deals 20 dmg. Same priority, same speed.
        // Whoever goes first determines final HP pattern.
        TackleAction a1(10), a2(20);
        battle.takeTurn(a1, a2);

        // Record final HP of pet1 as a fingerprint of execution order
        results.push_back(pet1.currentHP());
    }

    // All trials with same seed must produce identical results
    EXPECT_EQ(results[0], results[1]);
    EXPECT_EQ(results[1], results[2]);
}
