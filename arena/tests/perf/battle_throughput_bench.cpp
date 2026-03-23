// tests/perf/battle_throughput_bench.cpp
// Performance benchmark: Battle throughput (10K / 100K fixed-seed battles)
//
// Goal: Measure how many battles/second the engine can sustain
// Input: 10K and 100K identical fixed-seed battles (5 turns each)
// Metrics: Total wall-clock time, battles/sec, μs/battle
//
// This is a standalone executable, not gtest. Outputs structured results.

#include <chrono>
#include <cstdio>
#include <cstdint>

#include <battle/BattleSystem.h>
#include <battle/Action.h>
#include <entity/Pet.h>
#include <entity/Player.h>
#include <core/rng/rng.h>

namespace {

Species makeSpecies(int id, const char* name, BS bs) {
    return Species(id, name, {AttrType::Fire, AttrType::None}, bs);
}

Pet makePet(Species& sp) {
    IVData iv{31,31,31,31,31,31};
    EVData ev{0,0,0,0,0,0};
    Pet pet(&sp, iv, ev);
    pet.calcRealStat(sp.baseStats(), iv, ev, NatureType::Hardy, 100);
    return pet;
}

// Simple damage action
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

struct BenchResult {
    const char* name;
    int count;
    double totalMs;
    double perBattleUs;
    double battlesPerSec;
};

BenchResult runBattleBench(const char* name, int numBattles, int turnsPerBattle) {
    auto sp1 = makeSpecies(1, "Attacker", BS{100, 120, 80, 80, 80, 100});
    auto sp2 = makeSpecies(2, "Defender", BS{120, 80, 100, 80, 100, 70});

    auto start = std::chrono::high_resolution_clock::now();

    for (int b = 0; b < numBattles; ++b) {
        RNG::instance().reseed(static_cast<uint64_t>(b));

        Pet pet1 = makePet(sp1);
        Pet pet2 = makePet(sp2);

        Player::Roster r1{}, r2{};
        r1[0] = &pet1;
        r2[0] = &pet2;
        Player p1(r1, 0);
        Player p2(r2, 0);

        BattleSystem battle;
        battle.init(p1, p2);

        for (int t = 0; t < turnsPerBattle && !battle.isBattleOver(); ++t) {
            TackleAction a1(40), a2(35);
            battle.takeTurn(a1, a2);
        }
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    double perBattle = (ms / numBattles) * 1000.0; // μs
    double bps = numBattles / (ms / 1000.0);

    return {name, numBattles, ms, perBattle, bps};
}

void printResult(const BenchResult& r) {
    std::printf("  %-35s  %7d battles  %8.1f ms  %7.1f μs/battle  %10.0f battles/sec\n",
                r.name, r.count, r.totalMs, r.perBattleUs, r.battlesPerSec);
}

} // namespace

int main() {
    std::printf("=== RocoArena Battle Throughput Benchmark ===\n\n");

    // Suppress logger output for clean benchmark
    printResult(runBattleBench("5-turn battles (10K)", 10000, 5));
    printResult(runBattleBench("5-turn battles (100K)", 100000, 5));
    printResult(runBattleBench("20-turn battles (10K)", 10000, 20));
    printResult(runBattleBench("1-turn battles (100K)", 100000, 1));

    std::printf("\n=== Benchmark complete ===\n");
    return 0;
}
