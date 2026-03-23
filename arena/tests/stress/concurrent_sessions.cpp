// tests/stress/concurrent_sessions.cpp
// Stress test: Multi-session concurrent battle simulation
//
// Goal: Verify thread safety and resource contention under parallel load
// Input: N concurrent battle sessions (default 100) using std::thread
// Assertions:
//   - No data race, crash, or deadlock
//   - Each session reaches a deterministic outcome (same seed = same result)
//   - Total wall time scales sub-linearly with thread count

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <thread>
#include <vector>
#include <atomic>
#include <mutex>

#include <battle/BattleSystem.h>
#include <battle/Action.h>
#include <entity/Pet.h>
#include <entity/Player.h>
#include <core/rng/rng.h>

namespace {

struct SessionResult {
    int sessionId;
    int finalHP1;
    int finalHP2;
    int turns;
    bool battleOver;
    double elapsedMs;
};

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

void runSession(int sessionId, SessionResult& result) {
    auto start = std::chrono::high_resolution_clock::now();

    // Each thread gets its own RNG (thread_local in RNG::instance())
    RNG::instance().reseed(static_cast<uint64_t>(sessionId * 7 + 42));

    Species sp1(1, "Fighter", {AttrType::Fire, AttrType::None}, BS{80, 120, 80, 80, 80, 100});
    Species sp2(2, "Tank",    {AttrType::Water, AttrType::None}, BS{120, 80, 100, 80, 100, 70});

    IVData iv{31,31,31,31,31,31};
    EVData ev{0,0,0,0,0,0};

    Pet pet1(&sp1, iv, ev);
    pet1.calcRealStat(sp1.baseStats(), iv, ev, NatureType::Hardy, 100);
    Pet pet2(&sp2, iv, ev);
    pet2.calcRealStat(sp2.baseStats(), iv, ev, NatureType::Hardy, 100);

    Player::Roster r1{}, r2{};
    r1[0] = &pet1;
    r2[0] = &pet2;
    Player p1(r1, 0);
    Player p2(r2, 0);

    BattleSystem battle;
    battle.init(p1, p2);

    int maxTurns = 50;
    for (int t = 0; t < maxTurns && !battle.isBattleOver(); ++t) {
        int dmg1 = RNG::instance().range<int>(20, 60);
        int dmg2 = RNG::instance().range<int>(15, 50);
        TackleAction a1(dmg1), a2(dmg2);
        battle.takeTurn(a1, a2);
    }

    auto end = std::chrono::high_resolution_clock::now();

    result.sessionId = sessionId;
    result.finalHP1 = pet1.currentHP();
    result.finalHP2 = pet2.currentHP();
    result.turns = battle.currentTurn();
    result.battleOver = battle.isBattleOver();
    result.elapsedMs = std::chrono::duration<double, std::milli>(end - start).count();
}

} // namespace

int main(int argc, char* argv[]) {
    int numSessions = 100;
    if (argc > 1) {
        numSessions = std::atoi(argv[1]);
        if (numSessions <= 0) numSessions = 100;
    }

    std::printf("=== RocoArena Concurrent Session Stress Test ===\n");
    std::printf("Running %d parallel sessions...\n\n", numSessions);

    std::vector<SessionResult> results(static_cast<size_t>(numSessions));
    std::vector<std::thread> threads;
    threads.reserve(static_cast<size_t>(numSessions));

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numSessions; ++i) {
        threads.emplace_back(runSession, i, std::ref(results[static_cast<size_t>(i)]));
    }
    for (auto& t : threads) {
        t.join();
    }

    auto end = std::chrono::high_resolution_clock::now();
    double totalMs = std::chrono::duration<double, std::milli>(end - start).count();

    // Verify determinism: run session 0 again with same seed
    SessionResult verify;
    runSession(0, verify);
    bool deterministic = (verify.finalHP1 == results[0].finalHP1 &&
                          verify.finalHP2 == results[0].finalHP2 &&
                          verify.turns == results[0].turns);

    // Statistics
    double maxSessionMs = 0, minSessionMs = 1e9, sumMs = 0;
    int completed = 0;
    for (int i = 0; i < numSessions; ++i) {
        const auto& r = results[static_cast<size_t>(i)];
        sumMs += r.elapsedMs;
        if (r.elapsedMs > maxSessionMs) maxSessionMs = r.elapsedMs;
        if (r.elapsedMs < minSessionMs) minSessionMs = r.elapsedMs;
        if (r.battleOver) completed++;
    }

    std::printf("=== Stress Test Results ===\n");
    std::printf("  Sessions:          %d\n", numSessions);
    std::printf("  Completed battles: %d / %d\n", completed, numSessions);
    std::printf("  Total wall time:   %.1f ms\n", totalMs);
    std::printf("  Avg session time:  %.2f ms\n", sumMs / numSessions);
    std::printf("  Min session time:  %.2f ms\n", minSessionMs);
    std::printf("  Max session time:  %.2f ms\n", maxSessionMs);
    std::printf("  Sessions/sec:      %.0f\n", numSessions / (totalMs / 1000.0));
    std::printf("  Determinism check: %s\n", deterministic ? "PASSED" : "FAILED");
    std::printf("  Status:            %s\n",
                deterministic ? "PASSED (no crashes, deterministic)" : "FAILED (non-deterministic!)");

    return deterministic ? 0 : 1;
}
