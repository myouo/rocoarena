#include "scripter.h"

Scripter::Scripter() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    LOG_INFO(module(), "Lua initialized.");
}

bool Scripter::runScript(const std::string& filename) {
    try {
        lua.script_file(filename);
        LOG_INFO(module(), "Loaded script: ", filename);
        return true;
    } catch(const sol::error& e) {
        LOG_ERROR(module(), "Error running script ", filename, ": ", e.what());
        return false;
    }
}

bool Scripter::runString(const std::string& code) {
    try {
        lua.script(code);
        LOG_INFO(module(), "Executed code string.");
        return true;
    } catch(const sol::error& e) {
        LOG_ERROR(module(), "Error running code string: ", e.what());
        return false;
    }
}
