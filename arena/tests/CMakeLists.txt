include(FetchContent)
FetchContent_Declare(
  googletest
  URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# =============================================================================
# 1. Correctness tests (gtest-based)
# =============================================================================
set(ROCOARENA_TEST_SOURCES
  ${CMAKE_CURRENT_SOURCE_DIR}/dummy_test.cpp
  # Entity tests
  ${CMAKE_CURRENT_SOURCE_DIR}/entity/pet_stat_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/entity/pet_damage_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/entity/attr_chart_test.cpp
  # Battle tests
  ${CMAKE_CURRENT_SOURCE_DIR}/battle/buff_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/battle/buff_endturn_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/battle/battle_system_test.cpp
  # Core tests
  ${CMAKE_CURRENT_SOURCE_DIR}/core/scripter_test.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/core/asset_consistency_test.cpp
  # Session tests (deferred)
  ${CMAKE_CURRENT_SOURCE_DIR}/session/session_state_test.cpp
  # Regression tests
  ${CMAKE_CURRENT_SOURCE_DIR}/regression/golden_regression_test.cpp
)

add_executable(rocoarena_tests ${ROCOARENA_TEST_SOURCES})
target_link_libraries(rocoarena_tests PRIVATE rocoarena_core gtest_main)
target_include_directories(rocoarena_tests PRIVATE ${CMAKE_SOURCE_DIR}/src)
add_test(NAME rocoarena_tests COMMAND rocoarena_tests)

# =============================================================================
# 2. Performance benchmarks (standalone executables)
# =============================================================================

# Battle throughput benchmark
add_executable(bench_battle_throughput perf/battle_throughput_bench.cpp)
target_link_libraries(bench_battle_throughput PRIVATE rocoarena_core)
target_include_directories(bench_battle_throughput PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Lua execution benchmark
add_executable(bench_lua_execution perf/lua_execution_bench.cpp)
target_link_libraries(bench_lua_execution PRIVATE rocoarena_core)
target_include_directories(bench_lua_execution PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Stat calculation benchmark
add_executable(bench_stat_calc perf/stat_calc_bench.cpp)
target_link_libraries(bench_stat_calc PRIVATE rocoarena_core)
target_include_directories(bench_stat_calc PRIVATE ${CMAKE_SOURCE_DIR}/src)

# =============================================================================
# 3. Stress / soak tests (standalone executables)
# =============================================================================

# Long-round soak test
add_executable(stress_long_round stress/long_round_soak.cpp)
target_link_libraries(stress_long_round PRIVATE rocoarena_core)
target_include_directories(stress_long_round PRIVATE ${CMAKE_SOURCE_DIR}/src)

# Concurrent sessions stress test
add_executable(stress_concurrent stress/concurrent_sessions.cpp)
target_link_libraries(stress_concurrent PRIVATE rocoarena_core pthread)
target_include_directories(stress_concurrent PRIVATE ${CMAKE_SOURCE_DIR}/src)
