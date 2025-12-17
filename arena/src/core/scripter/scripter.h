#pragma once

/*
sudo apt install lua5.3 lublua5.3-dev
*/
#include <sol/sol.hpp>
#include <string>

#include <stdexcept>

#include "../logger/logger.h"

class Scripter {
public:
    static constexpr const char* module() { return "Scripter"; }

    // construct
    Scripter();
    // run Lua script
    bool runScript(const std::string& filename);
    // run Lua string
    bool runString(const std::string& code);
    // get Lua var
    template <typename T> T get(const std::string& name) const { return lua[name]; }
    // call Lua function
    template <typename Ret, typename... Args> 
    Ret call(const std::string& funcName, Args... args) {
        sol::function func = lua[funcName];
        if(!func.valid()) {
            LOG_ERROR(module(), "Function '", funcName, "' not found in Lua.");
            throw std::runtime_error("Function '" + funcName + "' not found in Lua.");
        }
        return func(args...);
   
    }

    //register cpp functions for Lua
    template <typename Func>
    void registerFunction(const std::string& name, Func f) {
        lua.set_function(name, f);
    }

    template <typename T>
    void set(const std::string& name, T val) {
        lua[name] = val;
    }

private:
    sol::state lua;
};
