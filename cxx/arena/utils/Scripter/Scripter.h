#pragma once

/*
sudo apt install lua5.3 lublua5.3-dev
*/
#include <sol/sol.hpp>
#include <string>
#include <iostream>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define RED_PRINT (std::cout << ANSI_COLOR_RED << "[Scripter] " << ANSI_COLOR_RESET)
#define RED_CERR (std::cerr << ANSI_COLOR_RED << "[Scripter] " << ANSI_COLOR_RESET)

class Scripter {
public:
    // construct
    Scripter();
    // run Lua script
    bool runScript(const std::string& filename);
    // run Lua string
    bool runString(const std::string& code);
    // get Lua var
    template <typename T>
    T get(const std::string& name) {
        return lua[name];
    };
    // call Lua function
    template <typename Ret, typename... Args> 
    Ret call(const std::string& funcName, Args... args) {
        sol::function func = lua[funcName];
        if(!func.valid()) {
            throw std::runtime_error("[Scripter] Function '" + funcName + "' not found in Lua.");
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
