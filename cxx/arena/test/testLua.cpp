#include <Scripter/Scripter.h>

int calc(int a, int b) {
    return a * b;
}
int main() {
    Scripter scr;
    std::string baseDir = "/root/rocoarena/cxx/arena/";
    std::string fileName = baseDir + "data/skills/test.lua";
    scr.runScript(fileName);
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
