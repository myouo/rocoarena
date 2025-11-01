#include <JSONLoader/JSONLoader.h>

int main() {
    JSONLoader loader;
    std::string dir = "/root/rocoarena/cxx/arena/test/testcases/test.json";
    loader.loadFromFile(dir);
    std::string any_ = loader.get<std::string>("title", "DEFAULT_PRINT");
    std::string any_1 = loader.get<std::string>("author", "DEFAULT_PRINT");
    int any_2 = loader.get<int>("published_year", 0);

    GREEN_PRINT << any_ << "\n" \
                << any_1 << "\n" \
                << any_2 << "\n";
    return 0;
}