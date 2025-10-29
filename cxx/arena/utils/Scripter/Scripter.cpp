#include "Scripter.h"

Scripter::Scripter() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    std::cout << "[Scripter] Lua initialized.\n";
}


bool Scripter::runScript(const std::string& filename) {
     try {
            lua.script_file(filename);
            std::cout << "[Scripter] Loaded Script: " << filename << '\n';
            return true;
        } catch(const sol::error& e) {
            std::cerr << "[Scripter] Error Running Script: " << e.what() << "\n";
            return false;
        }
}

bool Scripter::runString(const std::string& code) {
    try {
        lua.script(code);
        return true;
    } catch(const sol::error& e) {
        std::cerr << "[Scripter] Error Running Code: " << e.what() << "\n";
        return false;
    }
}

template <typename T>
T Scripter::get(const std::string& name) {
    return lua[name];
}
template <typename Ret, typename... Args> 
Ret Scripter::call(const std::string& funcName, Args... args) {
    sol::function func = lua[funcName];
    if(!func.valid()) {
        throw std::runtime_error("[Scripter] Function '" + funcName + "' not found in Lua.");
    }
    return func(args...);
}

template <typename Func>
void Scripter::registerFunction(const std::string&& name, Func f) {
    lua.set_function(name, f);
}