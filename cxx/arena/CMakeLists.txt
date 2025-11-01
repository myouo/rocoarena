cmake_minimum_required(VERSION 3.15)
project(TestJSON LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
add_compile_options(-Wall -O2)

# 1️⃣ 先创建可执行文件 target
add_executable(testJSON
    test/testJSON.cpp
    utils/JSONLoader/JSONLoader.cpp
)

# 2️⃣ 再添加头文件路径
target_include_directories(testJSON PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/utils
    /usr/include/lua5.3
)

# 3️⃣ 再添加链接库
target_link_libraries(testJSON PRIVATE lua5.3)
