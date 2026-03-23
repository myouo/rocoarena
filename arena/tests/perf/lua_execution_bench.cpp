// tests/perf/lua_execution_bench.cpp
// Performance benchmark: Lua skill execution throughput
//
// Goal: Measure Lua script loading + execution overhead
// Input: 10K/100K Lua function calls through the Scripter interface
// Metrics: Total time, μs/call, calls/sec

#include <chrono>
#include <cstdio>
#include <cstdint>
#include <string>

#include <core/scripter/scripter.h>

namespace {

struct BenchResult {
    const char* name;
    int count;
    double totalMs;
    double perCallUs;
    double callsPerSec;
};

void printResult(const BenchResult& r) {
    std::printf("  %-40s  %7d calls  %8.1f ms  %7.2f μs/call  %10.0f calls/sec\n",
                r.name, r.count, r.totalMs, r.perCallUs, r.callsPerSec);
}

// Benchmark: Lua function call round-trip
BenchResult benchLuaFunctionCall(int numCalls) {
    Scripter s;
    s.runString(R"(
        function calc_damage(base, multiplier)
            return math.floor(base * multiplier)
        end
    )");

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numCalls; ++i) {
        s.call<int>("calc_damage", 100, 1.5);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    return {"Lua calc_damage() call", numCalls, ms, (ms / numCalls) * 1000.0, numCalls / (ms / 1000.0)};
}

// Benchmark: Lua script loading (parsing + compiling)
BenchResult benchLuaScriptLoading(int numLoads) {
    const std::string script = R"(
        local hp = 100
        local damage = 30
        hp = hp - damage
        if hp < 0 then hp = 0 end
        result = hp
    )";

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numLoads; ++i) {
        Scripter s; // Fresh state each time (includes init overhead)
        s.runString(script);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    return {"Lua state init + script load", numLoads, ms, (ms / numLoads) * 1000.0, numLoads / (ms / 1000.0)};
}

// Benchmark: C++ -> Lua -> C++ round-trip with registered function
BenchResult benchLuaCppRoundTrip(int numCalls) {
    Scripter s;

    int accumulator = 0;
    s.registerFunction("add_to_acc", [&accumulator](int val) {
        accumulator += val;
    });

    s.runString(R"(
        function roundtrip(n)
            add_to_acc(n * 2)
        end
    )");

    auto start = std::chrono::high_resolution_clock::now();
    for (int i = 0; i < numCalls; ++i) {
        s.call<void>("roundtrip", i);
    }
    auto end = std::chrono::high_resolution_clock::now();

    double ms = std::chrono::duration<double, std::milli>(end - start).count();
    return {"C++ -> Lua -> C++ round-trip", numCalls, ms, (ms / numCalls) * 1000.0, numCalls / (ms / 1000.0)};
}

} // namespace

int main() {
    std::printf("=== RocoArena Lua Execution Benchmark ===\n\n");

    printResult(benchLuaFunctionCall(10000));
    printResult(benchLuaFunctionCall(100000));
    printResult(benchLuaScriptLoading(1000));
    printResult(benchLuaScriptLoading(10000));
    printResult(benchLuaCppRoundTrip(10000));
    printResult(benchLuaCppRoundTrip(100000));

    std::printf("\n=== Benchmark complete ===\n");
    return 0;
}
