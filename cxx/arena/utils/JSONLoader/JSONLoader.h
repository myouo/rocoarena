#pragma once

#include <nlohmann/json.hpp>
#include <optional>
#include <mutex>
#include <fstream>
#include <vector>
#include <iostream>

#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define GREEN_PRINT (std::cout << ANSI_COLOR_GREEN << "[JSONLoader] " << ANSI_COLOR_RESET)
#define GREEN_CERR (std::cerr << ANSI_COLOR_GREEN << "[JSONLoader] " << ANSI_COLOR_RESET)


class JSONLoader {
public:
    using json = nlohmann::json;
    JSONLoader() = default;
    explicit JSONLoader(const json& j);

    // load 
    void loadFromFile(const std::string& filePath);
    void loadFromString(const std::string& data);

    // save
    void saveToFile(const std::string& filePath, bool pretty = true) const;

    // query
    bool has(const std::string& path) const;
    void remove(const std::string& path);

    template <typename T>
    std::optional<T> getOpt(const std::string& path) const;

    template <typename T>
    T get(const std::string& path, const T &defaultVal) const;

    template <typename T>
    void set(const std::string& path, T &&value);

    json snapshot() const;
private:
    json root_;
    mutable std::mutex mtx_;

    static std::vector<std::string> splitPath(const std::string& path);

    const json* navigateConst(const std::vector<std::string>& parts) const;
    json* navigateCreate(const std::vector<std::string>& parts);

    static bool isIndexPart(const std::string& part, std::string& key, std::optional<size_t>& index);
};