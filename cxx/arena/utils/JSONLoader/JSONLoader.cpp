#include "JSONLoader.h"

#include <sstream>
#include <filesystem>
#include <stdexcept>
#include <regex>

using json = nlohmann::json;

JSONLoader::JSONLoader(const json& j) : root_(j) {}

void JSONLoader::loadFromFile(const std::string& filePath) {
    std::lock_guard lock(mtx_);
    std::ifstream ifs(filePath);
    if(!ifs) {
        throw std::runtime_error("Failed to open JSON file: " + filePath);
    }
    try {
        ifs >> root_;
        GREEN_PRINT << "opened JSON file.\n";
    } catch(const std::exception& e) {
        throw std::runtime_error(std::string("Failed to open JSON file: ") + e.what());
    }
}

void JSONLoader::loadFromString(const std::string& data) {
    std::lock_guard lock(mtx_);
    try {
        root_ = json::parse(data);
    } catch(const std::exception& e) {
        throw std::runtime_error(std::string("Failed to parse JSON string: ") + e.what());
    }
}

void JSONLoader::saveToFile(const std::string& filePath, bool pretty) const {
    std::lock_guard lock(mtx_);
    std::filesystem::path p(filePath);
    if(p.has_parent_path()) {
        std::filesystem::create_directories(p.parent_path());
    }
    
    std::ofstream ofs(filePath);
    if(!ofs) {
        throw std::runtime_error(std::string("Failed to open JSON file for writing: ") + filePath);
    }
    
    if(pretty) {
        ofs << root_.dump(4);
    } else {
        ofs << root_.dump();
    }
}

std::vector<std::string> JSONLoader::splitPath(const std::string& path) {
    std::vector<std::string> out;
    std::string buf;
    for(size_t i = 0; i < path.size(); ++i) {
        char c = path[i];
        if(c == '.') {
            if(!buf.empty()) {
                out.push_back(buf);
                buf.clear();
            }
        } else {
            buf.push_back(c);
        }
    }
    if(!buf.empty()) {
        out.push_back(buf);
    }
    return out;
}

bool JSONLoader::isIndexPart(const std::string& part, std::string& key, std::optional<size_t>& index) {
    static const std::regex re_full(R"(^([A-Za-z0-9_\-]*)\[(\d+)\]$)");
    std::smatch m;
    if(std::regex_match(part, m, re_full)) {
        key = m[1].str();
        if(key.empty()) {
            key = std::string();
        }
        index = static_cast<size_t>(std::stoll(m[2].str()));
        return true;
    }

    static const std::regex re_index(R"(^\[(\d+)\]$)");
    if(std::regex_match(part, m, re_index)) {
        key.clear();
        index = static_cast<size_t>(std::stoll(m[1].str()));
        return true;
    }

    key = part;
    index.reset();
    return false;
}

const json* JSONLoader::navigateConst(const std::vector<std::string>& parts) const {
    const json* cur = &root_;
    for(const auto &p : parts) {
        std::string key; std::optional<size_t> idx;
        isIndexPart(p, key, idx);
        if(!key.empty()) {
            if(!cur -> is_object()) return nullptr;
            auto it = cur -> find(key);
            if(it == cur -> end()) return nullptr;
            cur = &(*it);
        }
        if(idx.has_value()) {
            if(!cur -> is_array()) return nullptr;
            size_t i = idx.value();
            if(i >= cur -> size()) return nullptr;
            cur = &((*cur)[i]);
        }
    }
    return cur;
}

json* JSONLoader::navigateCreate(const std::vector<std::string>& parts) {
    json* cur = &root_;
    for(const auto& p : parts) {
        std::string key; std::optional<size_t> idx;
        isIndexPart(p, key, idx);
        if(!key.empty()) {
            if(!cur->is_object()) {
                *cur = json::object();
            }
            cur = &((*cur)[key]);
        }
        if(!idx.has_value()) {
            if(!cur -> is_array()) {
                *cur = json::array();
            }
            size_t i = idx.value();
            cur = &((*cur)[i]);
        }
    }
    return cur;
}

bool JSONLoader::has(const std::string& path) const {
    std::lock_guard lock(mtx_);
    auto parts = splitPath(path);
    return navigateConst(parts) != nullptr;
}

void JSONLoader::remove(const std::string& path) {
    std::lock_guard lock(mtx_);
    auto parts = splitPath(path);
    if(parts.empty()) {
        root_.clear();
        return;
    }

    std::vector<std::string> parentParts(parts.begin(), parts.end() - 1);
    const std::string& last = parts.back();
    json* parent = navigateCreate(parentParts);
    
    std::string key; std::optional<size_t> idx;
    isIndexPart(last, key, idx);

    if(!key.empty()) {
        if(parent -> is_object()) {
            parent -> erase(key);
        }
    } else if(idx.has_value()) {
        if(parent -> is_array()) {
            size_t i = idx.value();
            if(i < parent -> size()) {
                parent -> erase(parent -> begin() + 1);
            }
        }
    }
}

JSONLoader::json JSONLoader::snapshot() const {
    std::lock_guard lock(mtx_);
    return root_;
}

template <typename T>
std::optional<T> JSONLoader::getOpt(const std::string& path) const {
    std::lock_guard lock(mtx_);
    auto parts = splitPath(path);
    const json* p = navigateConst(parts);

    if(!p) return std::nullopt;
    try {
        return p -> get<T>();
    } catch(...) {
        return std::nullopt;
    }
}

template <typename T>
T JSONLoader::get(const std::string& path, const T& defaultVal) const {
    auto v = getOpt<T>(path);
    if(v) return *v;
    return defaultVal;
}

template <typename T>
void JSONLoader::set(const std::string& path, T &&value) {
    std::lock_guard lock(mtx_);
    auto parts = splitPath(path);
    json* p = navigateCreate(parts);
    *p = std::forward<T>(value);
}

template std::optional<int> JSONLoader::getOpt<int>(const std::string&) const;
template std::optional<long> JSONLoader::getOpt<long>(const std::string&) const;
template std::optional<double> JSONLoader::getOpt<double>(const std::string&) const;
template std::optional<std::string> JSONLoader::getOpt<std::string>(const std::string&) const;


template int JSONLoader::get<int>(const std::string&, const int&) const;
template long JSONLoader::get<long>(const std::string&, const long&) const;
template double JSONLoader::get<double>(const std::string&, const double&) const;
template std::string JSONLoader::get<std::string>(const std::string&, const std::string&) const;