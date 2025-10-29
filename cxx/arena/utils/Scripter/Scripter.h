#pragma once

#include <sol/sol.hpp>
#include <string>
#include <iostream>


class Scripter {
public:
    // 构造函数
    Scripter();
    // 执行 Lua 脚本
    bool runScript(const std::string& filename);
    // 执行 Lua 字符串脚本
    bool runString(const std::string& code);
    // get Lua var
    template <typename T>
    T get(const std::string& name) {
        return lua[name];
    }
    // call Lua function
    template <typename Ret, typename... Args> 
    Ret call(const std::string& funcName, Args... args); 
    //register cpp functions for Lua
    template <typename Func>
    void registerFunction(const std::string&& name, Func f);

private:
    sol::state lua;
};
