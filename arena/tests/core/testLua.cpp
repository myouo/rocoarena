#include <scripter/scripter.h>
#include <sstream>
#include <config.hpp>

int calc(int a, int b) {
    return a * b;
}

int main() {
    Scripter scr;
    std::stringstream ss;
    ss << DIR_CURRENT << "/testcases/test.lua";
    std::string dir = ss.str();
    
    RED_PRINT << dir << "\n";
    scr.runScript(dir);
    scr.runString("printf(1111)");
    int res = scr.call<int>("add", 10, 20);
    RED_PRINT << res << "\n";
    int x = scr.get<int>("x");
    RED_PRINT << x << "\n";
    scr.registerFunction("calc", calc);
    scr.runString(R"(
        print(calc(20, 34))
        )");
        
    return 0;
}

