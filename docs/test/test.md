# RocoArena Test Suite

## Prerequisites

```bash
sudo apt install libsqlite3-dev liblua5.3-dev
```

GoogleTest is fetched automatically via CMake `FetchContent` (v1.14.0).

---

## Build

```bash
cd arena
mkdir -p build && cd build
cmake ..
make -j4       # Builds ALL targets (app + tests + benchmarks + stress)
```

### Build Individual Targets

| Target | Command |
|--------|---------|
| Correctness tests | `make rocoarena_tests` |
| Battle throughput bench | `make bench_battle_throughput` |
| Stat calc bench | `make bench_stat_calc` |
| Lua execution bench | `make bench_lua_execution` |
| Long-round soak test | `make stress_long_round` |
| Concurrent stress test | `make stress_concurrent` |

All binaries are output to `build/tests/`.

---

## Run

### 1. Correctness Tests (gtest) — CI: every commit

```bash
./tests/rocoarena_tests
```

89 tests, ~2ms. Filter by suite:

```bash
./tests/rocoarena_tests --gtest_filter="BattleSystem*"
./tests/rocoarena_tests --gtest_filter="Scripter*"
./tests/rocoarena_tests --gtest_filter="GoldenRegression*"
```

### 2. Performance Benchmarks — CI: release branches only

```bash
./tests/bench_stat_calc             # StatCalc + AttrChart throughput
./tests/bench_lua_execution         # Lua call / init / round-trip
./tests/bench_battle_throughput     # 10K–100K battle simulations
```

### 3. Stress / Soak Tests — run manually or nightly

```bash
./tests/stress_long_round 10000     # 10K turns (default), arg = turn count
./tests/stress_concurrent 100       # 100 parallel sessions (default 100)
```

---

## Test Inventory

### Correctness Tests (`rocoarena_tests`)

| Suite | File | Tests | What It Covers |
|-------|------|------:|----------------|
| PetStatCalc | `entity/pet_stat_test.cpp` | 8 | HP/stat formulas, IV, EV, Nature modifiers |
| PetDamage | `entity/pet_damage_test.cpp` | 13 | HP invariants, damage modifiers, healing |
| AttrChart | `entity/attr_chart_test.cpp` | 13 | Type effectiveness (single/dual/god) |
| BuffStages | `battle/buff_test.cpp` | 8 | Stat stage clamping ±6, multipliers |
| BuffAilment | `battle/buff_test.cpp` | 7 | Ailment application, type immunity |
| BuffEndTurn | `battle/buff_endturn_test.cpp` | 8 | Burn/Poison/Toxic/Curse/Parasite settlement |
| BattleSystem | `battle/battle_system_test.cpp` | 10 | Turn order, fainted skip, lifecycle |
| Scripter | `core/scripter_test.cpp` | 9 | Lua load, syntax error, C++↔Lua API |
| AssetConsistency | `core/asset_consistency_test.cpp` | 7 | JSON parsing, DB schema, skills dir |
| SessionState | `session/session_state_test.cpp` | 3 | Deferred (GTEST_SKIP) |
| GoldenRegression | `regression/golden_regression_test.cpp` | 5 | Fixed-seed battle state snapshots |
| DummyTest | `dummy_test.cpp` | 1 | Smoke test |

### Performance Benchmarks (standalone)

| Executable | File | What It Measures |
|-----------|------|-----------------|
| `bench_stat_calc` | `perf/stat_calc_bench.cpp` | `calcRealStat` and `AttrChart` throughput |
| `bench_lua_execution` | `perf/lua_execution_bench.cpp` | Lua function calls, state init, C++↔Lua round-trip |
| `bench_battle_throughput` | `perf/battle_throughput_bench.cpp` | Battles/sec at 10K–100K scale |

### Stress / Soak Tests (standalone)

| Executable | File | What It Verifies |
|-----------|------|-----------------|
| `stress_long_round` | `stress/long_round_soak.cpp` | No state corruption over 10K+ turns |
| `stress_concurrent` | `stress/concurrent_sessions.cpp` | Thread safety, determinism under parallel load |

---

## Directory Layout

```
arena/tests/
├── CMakeLists.txt
├── dummy_test.cpp
├── entity/
│   ├── pet_stat_test.cpp
│   ├── pet_damage_test.cpp
│   └── attr_chart_test.cpp
├── battle/
│   ├── buff_test.cpp
│   ├── buff_endturn_test.cpp
│   └── battle_system_test.cpp
├── core/
│   ├── scripter_test.cpp
│   └── asset_consistency_test.cpp
├── session/
│   └── session_state_test.cpp
├── regression/
│   └── golden_regression_test.cpp
├── perf/
│   ├── battle_throughput_bench.cpp
│   ├── lua_execution_bench.cpp
│   └── stat_calc_bench.cpp
└── stress/
    ├── long_round_soak.cpp
    └── concurrent_sessions.cpp
```

---

## Adding New Tests

### gtest (correctness/regression)

1. Create `tests/<category>/your_test.cpp` with `#include <gtest/gtest.h>`
2. Add the file to `ROCOARENA_TEST_SOURCES` in `tests/CMakeLists.txt`
3. Link against `rocoarena_core` (already configured)

### Standalone (perf/stress)

1. Create `tests/perf/` or `tests/stress/` source with its own `main()`
2. Add to `tests/CMakeLists.txt`:

```cmake
add_executable(your_bench perf/your_bench.cpp)
target_link_libraries(your_bench PRIVATE rocoarena_core)
target_include_directories(your_bench PRIVATE ${CMAKE_SOURCE_DIR}/src)
```
