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
    
    scr.runScript(dir);
    scr.runString("printf(1111)");
    int res = scr.call<int>("add", 10, 20);
    int x = scr.get<int>("x");
    scr.registerFunction("calc", calc);
    scr.runString(R"(
        print(calc(20, 34))
        )");
        
    return 0;
}

