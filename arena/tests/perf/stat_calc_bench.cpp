// tests/perf/stat_calc_bench.cpp
// Performance benchmark: Entity stat calculation throughput
//
// Goal: Measure raw stat calculation speed (CPU-bound math)
// Input: 100K / 1M Pet stat calculations
// Metrics: Total time, μs/calc, calcs/sec

#include <chrono>
#include <cstdio>

#include <entity/Pet.h>
#include <entity/Species.h>

namespace {

struct BenchResult {
    const char* name;
    int count;
    double totalMs;
    double perCalcUs;
    double calcsPerSec;
};

void printResult(const BenchResult& r) {
    std::printf("  %-40s  %9d calcs  %8.1f ms  %7.3f μs/calc  %10.0f calcs/sec\n",
                r.name, r.count, r.totalMs, r.perCalcUs, r.calcsPerSec);
}

BenchResult benchStatCalc(int numCalcs) {
    Species sp(1, "BenchPet", {AttrType::Fire, AttrType::None}, BS{100, 120, 80, 110, 90, 95});

    auto start = std::chrono::high_resolution_clock::now();

    for (int i = 0; i < numCalcs; ++i) {
        IVData iv{31, 31, 31, 31, 31, 31};
        EVData ev{252, 0, 0, 252, 0, 0};
        Pet pet(&sp, iv, ev);
        pet.calcRealStat(sp.baseStats(), iv, ev, NatureType::Adamant, 100);

        // Prevent dead-code elimination
        volatile int hp = pet.maxHP();
        (void)hp;
    }

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    return {"Pet::calcRealStat", numCalcs, ms, (ms / numCalcs) * 1000.0, numCalcs / (ms / 1000.0)};
}

BenchResult benchAttrChart(int numLookups) {
    auto start = std::chrono::high_resolution_clock::now();

    volatile double result = 0;
    for (int i = 0; i < numLookups; ++i) {
        AttrType atk = static_cast<AttrType>(i % static_cast<int>(AttrType::COUNT));
        AttrType def1 = static_cast<AttrType>((i + 3) % static_cast<int>(AttrType::COUNT));
        AttrType def2 = static_cast<AttrType>((i + 7) % static_cast<int>(AttrType::COUNT));
        result = AttrChart::getAttrAdvantage(atk, std::array<AttrType, 2>{def1, def2});
    }
    (void)result;

    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    return {"AttrChart::getAttrAdvantage", numLookups, ms, (ms / numLookups) * 1000.0, numLookups / (ms / 1000.0)};
}

} // namespace

int main() {
    std::printf("=== RocoArena Stat Calculation Benchmark ===\n\n");

    printResult(benchStatCalc(100000));
    printResult(benchStatCalc(1000000));
    printResult(benchAttrChart(100000));
    printResult(benchAttrChart(1000000));

    std::printf("\n=== Benchmark complete ===\n");
    return 0;
}
