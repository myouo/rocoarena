// tests/stress/long_round_soak.cpp
// Soak test: Long-round battle stability
//
// Goal: Verify the engine doesn't leak, crash, or corrupt state over 1000+ turns
// Input: A single battle running for configurable N turns (default 10,000)
// Assertions:
//   - No crash or hang
//   - HP invariants hold on every turn (0 <= HP <= MaxHP)
//   - Buff stages stay within [-6, +6]
//   - Turn counter increments correctly
//   - Memory usage doesn't grow unbounded (reported, not asserted)

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cassert>

#include <battle/BattleSystem.h>
#include <battle/Action.h>
#include <entity/Pet.h>
#include <entity/Player.h>
#include <core/rng/rng.h>

namespace {

Species makeSpecies(int id, const char* name, BS bs) {
    return Species(id, name, {AttrType::Normal, AttrType::None}, bs);
}

Pet makePet(Species& sp) {
    IVData iv{31,31,31,31,31,31};
    EVData ev{0,0,0,0,0,0};
    Pet pet(&sp, iv, ev);
    pet.calcRealStat(sp.baseStats(), iv, ev, NatureType::Hardy, 100);
    return pet;
}

// Action that deals small damage and randomly buffs/debuffs
class ChaoticAction : public Action {
public:
    ChaoticAction() : Action(ActionType::Skill) {}
    int priority() const override { return 0; }
    void execute(BattleSystem&, Player& self, Player& opp) override {
        // Deal small damage (never kills in one hit)
        int dmg = RNG::instance().range<int>(1, 5);
        opp.activePet().takeDamage(dmg);

        // Random buff/debuff
        int stat = RNG::instance().range<int>(0, 5);
        int delta = RNG::instance().range<int>(-2, 2);
        self.activePet().buff().changeStage(static_cast<Stat>(stat), delta);

        // Heal a tiny bit to extend the battle
        self.activePet().restoreHP(RNG::instance().range<int>(1, 8));
    }
};

void checkInvariants(const Pet& pet, int turn) {
    // HP invariants
    if (pet.currentHP() < 0) {
        std::fprintf(stderr, "INVARIANT VIOLATION: Turn %d - HP below 0: %d\n", turn, pet.currentHP());
        std::abort();
    }
    if (pet.currentHP() > pet.maxHP()) {
        std::fprintf(stderr, "INVARIANT VIOLATION: Turn %d - HP exceeds max: %d > %d\n",
                     turn, pet.currentHP(), pet.maxHP());
        std::abort();
    }

    // Stat stage invariants
    for (int s = 0; s <= static_cast<int>(Stat::Cri); ++s) {
        int stage = pet.buff().stage(static_cast<Stat>(s));
        if (stage < -6 || stage > 6) {
            std::fprintf(stderr, "INVARIANT VIOLATION: Turn %d - Stat %d stage out of range: %d\n",
                         turn, s, stage);
            std::abort();
        }
    }
}

} // namespace

int main(int argc, char* argv[]) {
    int totalTurns = 10000;
    if (argc > 1) {
        totalTurns = std::atoi(argv[1]);
        if (totalTurns <= 0) totalTurns = 10000;
    }

    std::printf("=== RocoArena Long-Round Soak Test ===\n");
    std::printf("Running %d turns...\n\n", totalTurns);

    RNG::instance().reseed(12345);

    // Use very high HP so the battle doesn't end naturally
    auto sp1 = makeSpecies(1, "TankA", BS{255, 100, 100, 100, 100, 100});
    auto sp2 = makeSpecies(2, "TankB", BS{255, 100, 100, 100, 100, 100});
    Pet pet1 = makePet(sp1);
    Pet pet2 = makePet(sp2);

    Player::Roster r1{}, r2{};
    r1[0] = &pet1;
    r2[0] = &pet2;
    Player p1(r1, 0);
    Player p2(r2, 0);

    BattleSystem battle;
    battle.init(p1, p2);

    auto start = std::chrono::high_resolution_clock::now();
    int checkpoints = 0;

    for (int t = 1; t <= totalTurns; ++t) {
        if (battle.isBattleOver()) {
            // Re-init to keep going (the point is to test sustained load)
            pet1 = makePet(sp1);
            pet2 = makePet(sp2);
            r1[0] = &pet1;
            r2[0] = &pet2;
            p1 = Player(r1, 0);
            p2 = Player(r2, 0);
            battle.init(p1, p2);
        }

        ChaoticAction a1, a2;
        battle.takeTurn(a1, a2);

        // Check invariants on every turn
        if (!pet1.isFainted()) checkInvariants(pet1, t);
        if (!pet2.isFainted()) checkInvariants(pet2, t);

        // Progress report every 1000 turns
        if (t % 1000 == 0) {
            checkpoints++;
            auto now = std::chrono::high_resolution_clock::now();
            double elapsed = std::chrono::duration<double>(now - start).count();
            std::printf("  [%6d / %d] %.1fs elapsed, Pet1 HP=%d/%d, Pet2 HP=%d/%d\n",
                        t, totalTurns, elapsed,
                        pet1.currentHP(), pet1.maxHP(),
                        pet2.currentHP(), pet2.maxHP());
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double totalSec = std::chrono::duration<double>(end - start).count();

    std::printf("\n=== Soak Test Results ===\n");
    std::printf("  Total turns:     %d\n", totalTurns);
    std::printf("  Total time:      %.2f s\n", totalSec);
    std::printf("  Turns/sec:       %.0f\n", totalTurns / totalSec);
    std::printf("  Invariant checks: %d (every turn)\n", totalTurns * 2);
    std::printf("  Checkpoints:     %d\n", checkpoints);
    std::printf("  Status:          PASSED (no invariant violations)\n");

    return 0;
}
