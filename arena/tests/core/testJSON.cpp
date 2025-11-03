#include <json_loader/json_loader.h>

int main() {
    JSONLoader loader;
    std::string dir = "../testcases/test.json";
    loader.loadFromFile(dir);
    std::string any_ = loader.get<std::string>("title", "DEFAULT_PRINT");
    std::string any_1 = loader.get<std::string>("author", "DEFAULT_PRINT");
    int any_2 = loader.get<int>("published_year", 0);

    GREEN_PRINT << any_ << "\n";
    GREEN_PRINT << any_1 << "\n";
    GREEN_PRINT << any_2 << "\n";
    return 0;
}
