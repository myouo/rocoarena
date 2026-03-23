cmake_minimum_required(VERSION 3.15)

project(rocoarena LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

option(ROCOARENA_ENABLE_WARNINGS "Enable extra compiler warnings" ON)

if(ROCOARENA_ENABLE_WARNINGS)
  add_compile_options(-Wall -Wextra -Wpedantic -O2)
endif()

find_package(SQLite3 QUIET)
find_package(Lua 5.3 QUIET)

set(ROCOARENA_FETCH_LUA OFF)
set(ROCOARENA_FETCH_SQLITE3 OFF)

if(NOT TARGET Lua::Lua)
  if(LUA_FOUND OR Lua_FOUND)
    add_library(Lua::Lua INTERFACE IMPORTED)
    set_target_properties(Lua::Lua PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${LUA_INCLUDE_DIR}"
      INTERFACE_LINK_LIBRARIES "${LUA_LIBRARIES}"
    )
  else()
    set(ROCOARENA_FETCH_LUA ON)
  endif()
endif()

if(NOT TARGET SQLite::SQLite3)
  if(SQLite3_FOUND)
    add_library(SQLite::SQLite3 INTERFACE IMPORTED)
    set_target_properties(SQLite::SQLite3 PROPERTIES
      INTERFACE_INCLUDE_DIRECTORIES "${SQLite3_INCLUDE_DIRS}"
      INTERFACE_LINK_LIBRARIES "${SQLite3_LIBRARIES}"
    )
  else()
    set(ROCOARENA_FETCH_SQLITE3 ON)
  endif()
endif()

if(ROCOARENA_FETCH_LUA OR ROCOARENA_FETCH_SQLITE3)
  include(FetchContent)
endif()

if(ROCOARENA_FETCH_LUA)
  FetchContent_Declare(
    lua
    URL https://www.lua.org/ftp/lua-5.3.6.tar.gz
  )
  FetchContent_MakeAvailable(lua)
  add_library(lua STATIC
    ${lua_SOURCE_DIR}/src/lapi.c
    ${lua_SOURCE_DIR}/src/lauxlib.c
    ${lua_SOURCE_DIR}/src/lbaselib.c
    ${lua_SOURCE_DIR}/src/lcode.c
    ${lua_SOURCE_DIR}/src/lcorolib.c
    ${lua_SOURCE_DIR}/src/lctype.c
    ${lua_SOURCE_DIR}/src/ldblib.c
    ${lua_SOURCE_DIR}/src/ldebug.c
    ${lua_SOURCE_DIR}/src/ldo.c
    ${lua_SOURCE_DIR}/src/ldump.c
    ${lua_SOURCE_DIR}/src/lfunc.c
    ${lua_SOURCE_DIR}/src/lgc.c
    ${lua_SOURCE_DIR}/src/linit.c
    ${lua_SOURCE_DIR}/src/liolib.c
    ${lua_SOURCE_DIR}/src/llex.c
    ${lua_SOURCE_DIR}/src/lmathlib.c
    ${lua_SOURCE_DIR}/src/lmem.c
    ${lua_SOURCE_DIR}/src/loadlib.c
    ${lua_SOURCE_DIR}/src/lobject.c
    ${lua_SOURCE_DIR}/src/lopcodes.c
    ${lua_SOURCE_DIR}/src/loslib.c
    ${lua_SOURCE_DIR}/src/lparser.c
    ${lua_SOURCE_DIR}/src/lstate.c
    ${lua_SOURCE_DIR}/src/lstring.c
    ${lua_SOURCE_DIR}/src/lstrlib.c
    ${lua_SOURCE_DIR}/src/ltable.c
    ${lua_SOURCE_DIR}/src/ltablib.c
    ${lua_SOURCE_DIR}/src/ltm.c
    ${lua_SOURCE_DIR}/src/lundump.c
    ${lua_SOURCE_DIR}/src/lutf8lib.c
    ${lua_SOURCE_DIR}/src/lvm.c
    ${lua_SOURCE_DIR}/src/lzio.c
  )
  add_library(Lua::Lua ALIAS lua)
  target_include_directories(lua PUBLIC ${lua_SOURCE_DIR}/src)
endif()

if(ROCOARENA_FETCH_SQLITE3)
  FetchContent_Declare(
    sqlite3
    URL https://www.sqlite.org/2023/sqlite-amalgamation-3430200.zip
  )
  FetchContent_MakeAvailable(sqlite3)
  add_library(sqlite3 STATIC ${sqlite3_SOURCE_DIR}/sqlite3.c)
  add_library(SQLite::SQLite3 ALIAS sqlite3)
  target_include_directories(sqlite3 PUBLIC ${sqlite3_SOURCE_DIR})
endif()

enable_testing()
add_subdirectory(src)

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests/CMakeLists.txt")
  add_subdirectory(tests)
endif()
