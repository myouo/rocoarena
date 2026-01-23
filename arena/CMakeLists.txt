`cmake_minimum_required(VERSION 3.15)

project(rocoarena LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)


option(ROCOARENA_ENABLE_WARNINGS "Enable extra compiler warnings" ON)

if(ROCOARENA_ENABLE_WARNINGS)
  add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

# Fetch SQLite3 and Lua 5.3
include(FetchContent)

# --- SQLite3 ---
# 声明 SQLite3 的源，它自带 CMakeLists.txt
FetchContent_Declare(
  sqlite3_dep
  GIT_REPOSITORY https://github.com/sqlite/sqlite.git
  GIT_TAG        version-3.45.1
)
# 下载并使其目标 'sqlite3' 可用
FetchContent_MakeAvailable(sqlite3_dep)

# --- Lua 5.3 ---
FetchContent_Declare(
  lua_dep
  GIT_REPOSITORY https://github.com/lua/lua.git
  GIT_TAG        v5.3.6
)

FetchContent_Populate(lua_dep)

file(GLOB LUA_SOURCES "${lua_dep_SOURCE_DIR}/src/*.c")
list(FILTER LUA_SOURCES EXCLUDE REGEX ".*/lua(c)?\\.c$") # 排除解释器入口
add_library(lua_lib STATIC ${LUA_SOURCES})
target_include_directories(lua_lib PUBLIC "${lua_dep_SOURCE_DIR}/src")

# 在Linux/macOS上，Lua 可能需要链接数学库
if(UNIX AND NOT APPLE)
    target_link_libraries(lua_lib PUBLIC m)
endif()

add_library(SQLite::SQLite3 ALIAS sqlite3)

add_library(Lua::Lua ALIAS lua_lib)

add_subdirectory(src)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()