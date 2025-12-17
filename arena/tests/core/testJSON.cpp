#include <sstream>

#include <config.hpp>
#include <json_loader/json_loader.h>

int main() {
    JSONLoader loader;
    std::stringstream ss;
    ss << DIR_CURRENT << "/testcases/test.json";
    std::string dir = ss.str();
    loader.loadFromFile(dir);
    std::string any_ = loader.get<std::string>("title", "DEFAULT_PRINT");
    std::string any_1 = loader.get<std::string>("author", "DEFAULT_PRINT");
    int any_2 = loader.get<int>("published_year", 0);
    return 0;
}
