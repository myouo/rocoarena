set(ROCOARENA_SOURCES
  core/json_loader/json_loader.cpp
  core/scripter/scripter.cpp
  core/database/database.cpp
  battle/Action.cpp
  battle/Buff.cpp
  battle/BattleSystem.cpp
  battle/SkillAction.cpp
  skill/SkillPool.cpp
  skill/SkillRegistry.cpp
  entity/Player.cpp
  entity/Pet.cpp
)

add_library(rocoarena_core STATIC ${ROCOARENA_SOURCES})
add_library(rocoarena::core ALIAS rocoarena_core)

target_compile_features(rocoarena_core PUBLIC cxx_std_17)

target_include_directories(rocoarena_core
  PUBLIC
    ${PROJECT_SOURCE_DIR}/src
    ${PROJECT_SOURCE_DIR}/src/core
    ${PROJECT_SOURCE_DIR}/src/entity
    ${PROJECT_SOURCE_DIR}/src/battle
    ${PROJECT_SOURCE_DIR}/src/skill
    ${PROJECT_SOURCE_DIR}/external
)

target_link_libraries(rocoarena_core
  PUBLIC
    SQLite::SQLite3
    Lua::Lua
)

add_executable(rocoarena main.cpp)
target_link_libraries(rocoarena PRIVATE rocoarena_core)
