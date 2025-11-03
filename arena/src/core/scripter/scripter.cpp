#include "scripter.h"

Scripter::Scripter() {
    lua.open_libraries(sol::lib::base, sol::lib::math, sol::lib::string, sol::lib::table);
    RED_PRINT << "Lua initialized.\n";
}


bool Scripter::runScript(const std::string& filename) {
     try {
            lua.script_file(filename);
            RED_PRINT << "Loaded Script: " << filename << '\n';
            return true;
        } catch(const sol::error& e) {
            RED_PRINT << "Running Script: " << e.what() << "\n";
            return false;
        }
}

bool Scripter::runString(const std::string& code) {
    try {
        lua.script(code);
        return true;
    } catch(const sol::error& e) {
        RED_CERR << "Error Running Code: " << e.what() << "\n";
        return false;
    }
}
