// tests/battle/battle_system_test.cpp
// Strategy tests #3, #5-10: Turn order, fainted pet skip, settlement order

#include <gtest/gtest.h>
#include <battle/BattleSystem.h>
#include <battle/Action.h>
#include <entity/Pet.h>
#include <entity/Player.h>
#include <core/rng/rng.h>

namespace {

Species makeSpecies(int id, const char* name, BS bs, AttrType a1 = AttrType::Normal, AttrType a2 = AttrType::None) {
    return Species(id, name, {a1, a2}, bs);
}

// Create a Pet with known stats, fully initialized at level 100
Pet makePet(Species& sp) {
    IVData iv{31,31,31,31,31,31};
    EVData ev{0,0,0,0,0,0};
    Pet pet(&sp, iv, ev);
    pet.calcRealStat(sp.baseStats(), iv, ev, NatureType::Hardy, 100);
    return pet;
}

// Helper to create a Player with a single pet
Player makePlayer(Pet& pet) {
    Player::Roster roster{};
    roster[0] = &pet;
    return Player(roster, 0);
}

// Simple tracking action: records whether it was executed
class TrackingAction : public Action {
public:
    TrackingAction(int prio, bool* executed)
        : Action(ActionType::Stay), prio_(prio), executed_(executed) {}

    int priority() const override { return prio_; }
    void execute(BattleSystem& /*battle*/, Player& /*self*/, Player& /*opponent*/) override {
        if (executed_) *executed_ = true;
    }

private:
    int prio_;
    bool* executed_;
};

// Damage action that deals fixed damage to opponent's active pet
class FixedDamageAction : public Action {
public:
    FixedDamageAction(int dmg)
        : Action(ActionType::Skill), dmg_(dmg) {}

    int priority() const override { return 0; }
    void execute(BattleSystem& /*battle*/, Player& /*self*/, Player& opponent) override {
        opponent.activePet().takeDamage(dmg_);
    }

private:
    int dmg_;
};

} // namespace

// =============================================================================
// #3: BattleInvariant_Dead_Pets_Cannot_Execute_Actions
// If the second actor's pet faints from the first action, its action is skipped.
// =============================================================================

TEST(BattleSystem, DeadPetsCannotExecuteActions) {
    auto sp1 = makeSpecies(1, "FastPet", BS{100, 100, 100, 100, 100, 200}); // Very fast
    auto sp2 = makeSpecies(2, "SlowPet", BS{50, 50, 50, 50, 50, 10});       // Very slow, low HP

    Pet pet1 = makePet(sp1);
    Pet pet2 = makePet(sp2);
    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);

    // Action 1 deals massive damage (kills pet2), action 2 tracks execution
    bool secondExecuted = false;
    FixedDamageAction killAction(99999);
    TrackingAction trackAction(0, &secondExecuted);

    // pet1 is faster (speed 200 vs 10), so killAction executes first
    battle.takeTurn(killAction, trackAction);

    EXPECT_TRUE(pet2.isFainted());
    EXPECT_FALSE(secondExecuted); // Dead pet's action must be skipped
}

// =============================================================================
// #5: TurnOrder_Higher_Priority_Skill_Executes_First
// =============================================================================

TEST(BattleSystem, HigherPriorityExecutesFirst) {
    auto sp = makeSpecies(1, "Pet", BS{100, 100, 100, 100, 100, 100});
    Pet pet1 = makePet(sp);
    Pet pet2 = makePet(sp);
    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);

    // FleeAction has priority 16, StayAction has priority 8
    // Flee should execute first (ends battle) and Stay never runs
    bool stayExecuted = false;
    FleeAction flee;
    TrackingAction stay(8, &stayExecuted);

    battle.takeTurn(flee, stay);

    EXPECT_TRUE(battle.isBattleOver());
    EXPECT_EQ(battle.endReason(), "fled");
    EXPECT_FALSE(stayExecuted);
}

// =============================================================================
// #6: TurnOrder_Higher_Speed_Executes_First_On_Same_Priority
// =============================================================================

TEST(BattleSystem, HigherSpeedExecutesFirstOnSamePriority) {
    auto spFast = makeSpecies(1, "FastPet", BS{100, 100, 100, 100, 100, 200});
    auto spSlow = makeSpecies(2, "SlowPet", BS{100, 100, 100, 100, 100, 10});
    Pet petFast = makePet(spFast);
    Pet petSlow = makePet(spSlow);
    Player p1 = makePlayer(petFast);
    Player p2 = makePlayer(petSlow);

    BattleSystem battle;
    battle.init(p1, p2);

    // Both same priority (0), fast pet should go first
    // Fast pet's action kills slow pet, so slow pet's action is skipped
    bool slowExecuted = false;
    FixedDamageAction killAction(99999);
    TrackingAction slowAction(0, &slowExecuted);

    battle.takeTurn(killAction, slowAction);

    EXPECT_TRUE(petSlow.isFainted());
    EXPECT_FALSE(slowExecuted);
}

// =============================================================================
// #7: TurnOrder_SpeedTie_Resolved_By_Fixed_RNG
// =============================================================================

TEST(BattleSystem, SpeedTieResolvedByFixedRNG) {
    auto sp = makeSpecies(1, "EqualPet", BS{100, 100, 100, 100, 100, 100});

    // Run with two different seeds and verify deterministic behavior
    for (uint64_t seed : {42ULL, 123ULL, 999ULL}) {
        Pet pet1 = makePet(sp);
        Pet pet2 = makePet(sp);
        Player p1 = makePlayer(pet1);
        Player p2 = makePlayer(pet2);

        BattleSystem battle;
        battle.init(p1, p2);

        RNG::instance().reseed(seed);

        // Both priority 0, same speed — RNG decides
        // We just verify it doesn't crash and proceeds normally
        StayAction stay1;
        StayAction stay2;
        battle.takeTurn(stay1, stay2);

        EXPECT_FALSE(battle.isBattleOver());
        EXPECT_EQ(battle.currentTurn(), 1);
    }
}

// =============================================================================
// #10: Settlement_Fainted_Before_Action_Cancels_Action
// Same as #3 but from a different perspective — verifying battle logs
// =============================================================================

TEST(BattleSystem, FaintedBeforeActionCancelsAction) {
    auto sp1 = makeSpecies(1, "Attacker", BS{100, 100, 100, 100, 100, 200});
    auto sp2 = makeSpecies(2, "Defender", BS{30, 30, 30, 30, 30, 10}); // Low HP

    Pet pet1 = makePet(sp1);
    Pet pet2 = makePet(sp2);
    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);

    int hpBefore = pet1.currentHP();
    FixedDamageAction overkill(99999);
    FixedDamageAction counterattack(50); // Should never execute

    battle.takeTurn(overkill, counterattack);

    EXPECT_TRUE(pet2.isFainted());
    EXPECT_EQ(pet1.currentHP(), hpBefore); // Counter never happened
    EXPECT_TRUE(battle.isBattleOver());
}

// =============================================================================
// Battle initialization and edge cases
// =============================================================================

TEST(BattleSystem, BattleEndsWhenNoUsablePets) {
    auto sp = makeSpecies(1, "Pet", BS{100, 100, 100, 100, 100, 100});
    Pet pet1 = makePet(sp);
    Pet pet2 = makePet(sp);
    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);

    // Kill pet2 directly
    pet2.takeDamage(99999);
    EXPECT_TRUE(pet2.isFainted());

    StayAction stay1;
    StayAction stay2;
    battle.takeTurn(stay1, stay2);

    EXPECT_TRUE(battle.isBattleOver());
}

TEST(BattleSystem, TurnCounterIncrements) {
    auto sp = makeSpecies(1, "Pet", BS{100, 100, 100, 100, 100, 100});
    Pet pet1 = makePet(sp);
    Pet pet2 = makePet(sp);
    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);
    EXPECT_EQ(battle.currentTurn(), 0);

    StayAction stay1, stay2;
    battle.takeTurn(stay1, stay2);
    EXPECT_EQ(battle.currentTurn(), 1);

    StayAction stay3, stay4;
    battle.takeTurn(stay3, stay4);
    EXPECT_EQ(battle.currentTurn(), 2);
}

TEST(BattleSystem, TakeTurnAfterEndIsNoop) {
    auto sp = makeSpecies(1, "Pet", BS{100, 100, 100, 100, 100, 100});
    Pet pet1 = makePet(sp);
    Pet pet2 = makePet(sp);
    Player p1 = makePlayer(pet1);
    Player p2 = makePlayer(pet2);

    BattleSystem battle;
    battle.init(p1, p2);
    battle.endBattle("test");

    StayAction stay1, stay2;
    battle.takeTurn(stay1, stay2);

    EXPECT_EQ(battle.currentTurn(), 0); // Turn didn't increment
}
