cmake_minimum_required(VERSION 3.15)

project(rocoarena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ROCOARENA_ENABLE_WARNINGS "Enable extra compiler warnings" ON)

if(ROCOARENA_ENABLE_WARNINGS)
  add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

find_package(SQLite3 REQUIRED)
find_package(Lua 5.3 REQUIRED)

# Backward compatibility for environments where the imported targets are missing.
if(NOT TARGET Lua::Lua)
  add_library(Lua::Lua INTERFACE IMPORTED)
  set_target_properties(Lua::Lua PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
    INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
  )
endif()

if(NOT TARGET SQLite::SQLite3)
  add_library(SQLite::SQLite3 INTERFACE IMPORTED)
  set_target_properties(SQLite::SQLite3 PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIRS}"
    INTERFACE_LINK_LIBRARIES "${SQLite3_LIBRARIES}"
  )
endif()

add_subdirectory(src)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
