cmake_minimum_required(VERSION 3.15)
project(TESTS LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -O2)

# 公共部分
set(COMMON_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/core
    ${CMAKE_CURRENT_SOURCE_DIR}/../external
    /usr/include/lua5.3
)
set(COMMON_LIBS
    lua5.3
)

set(ENTITY_INCLUDES
    ${CMAKE_CURRENT_SOURCE_DIR}/../src/entity
)

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/config.hpp.in
    ${CMAKE_CURRENT_BINARY_DIR}/config.hpp
)

# testJSON
add_executable(testJSON
    core/testJSON.cpp
    ../src/core/json_loader/json_loader.cpp
)
target_include_directories(testJSON PRIVATE ${COMMON_INCLUDES})
# target_link_libraries(testJSON PRIVATE ${COMMON_LIBS})

# testLua
add_executable(testLua
    core/testLua.cpp
    ../src/core/scripter/scripter.cpp
)

# testEntity
add_executable(testEntity
    entity/testEntity.cpp
    ../src/entity/Pet.cpp
)

target_include_directories(testLua PRIVATE ${COMMON_INCLUDES} ${CMAKE_CURRENT_BINARY_DIR})
target_include_directories(testEntity PRIVATE ${ENTITY_INCLUDES})
target_link_libraries(testLua PRIVATE ${COMMON_LIBS})
