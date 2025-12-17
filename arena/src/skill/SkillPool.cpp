#include "SkillPool.h"

#include <filesystem>
#include <fstream>

using nlohmann::json;

namespace {
bool isValidSkillType(int value) {
    return value >= static_cast<int>(SkillType::Physical) && value <= static_cast<int>(SkillType::Status);
}
} // namespace

bool SkillPool::loadAllFromDirectory(const std::string& dir, std::string* error) {
    namespace fs = std::filesystem;
    skills_.clear();

    fs::path root(dir);
    std::error_code ec;
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        if (error) *error = "directory not found: " + dir;
        return false;
    }

    std::vector<std::shared_ptr<SkillBase>> parsed;

    for (const auto& entry : fs::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_regular_file() || entry.path().extension() != ".json") continue;

        std::ifstream in(entry.path());
        if (!in.is_open()) {
            if (error) *error = "failed to open file: " + entry.path().string();
            return false;
        }

        json data;
        try {
            in >> data;
        } catch (const std::exception& e) {
            if (error) *error = entry.path().string() + " invalid json: " + e.what();
            return false;
        }

        auto addContextError = [&](const std::string& msg) {
            if (error) *error = entry.path().string() + ": " + msg;
        };

        if (data.is_array()) {
            for (const auto& item : data) {
                if (!parseSkillJson(item, parsed, error)) {
                    addContextError(error ? *error : "parse error");
                    return false;
                }
            }
        } else if (data.is_object()) {
            if (!parseSkillJson(data, parsed, error)) {
                addContextError(error ? *error : "parse error");
                return false;
            }
        } else {
            addContextError("skill json root must be array or object");
            return false;
        }
    }

    if (ec) {
        if (error) *error = "failed to iterate directory: " + dir;
        return false;
    }

    for (auto& s : parsed) {
        if (skills_.count(s->id())) {
            if (error) *error = "duplicate skill id: " + std::to_string(s->id());
            skills_.clear();
            return false;
        }
        skills_.emplace(s->id(), std::move(s));
    }

    return !skills_.empty();
}

const std::shared_ptr<SkillBase> SkillPool::getShared(int id) const {
    auto it = skills_.find(id);
    if (it == skills_.end()) return nullptr;
    return it->second;
}

bool SkillPool::buildSubsetRegistry(const std::vector<int>& ids, SkillRegistry& registry, std::string* error) const {
    std::vector<std::shared_ptr<SkillBase>> subset;
    subset.reserve(ids.size());

    for (int id : ids) {
        auto s = getShared(id);
        if (!s) {
            if (error) *error = "skill id not found in pool: " + std::to_string(id);
            return false;
        }
        subset.push_back(s);
    }

    return registry.loadShared(std::move(subset), error);
}

AttrType SkillPool::parseAttr(const std::string& attrName) const {
    auto itEn = AttrFromEN.find(attrName);
    if (itEn != AttrFromEN.end()) return itEn->second;

    auto itZh = AttrFromZH.find(attrName);
    if (itZh != AttrFromZH.end()) return itZh->second;

    if (attrName == "None") return AttrType::None;
    return AttrType::None;
}

bool SkillPool::parseSkillJson(const json& j, std::vector<std::shared_ptr<SkillBase>>& skills,
                               std::string* error) const {
    if (!j.contains("id") || !j.contains("name") || !j.contains("description") || !j.contains("skillType")) {
        if (error) *error = "skill json missing required fields (id/name/description/skillType)";
        return false;
    }

    int id = j.value("id", 0);
    auto name = std::make_shared<std::string>(j.value("name", ""));
    auto description = std::make_shared<std::string>(j.value("description", ""));

    int typeValue = j.value("skillType", 0);
    if (!isValidSkillType(typeValue)) {
        if (error) *error = "invalid skillType value for skill id " + std::to_string(id);
        return false;
    }
    SkillType type = static_cast<SkillType>(typeValue);

    std::string attrName = j.value("type", "");
    AttrType attr = parseAttr(attrName);
    if (attr == AttrType::None) {
        if (error) *error = "invalid attr/type for skill id " + std::to_string(id) + ": " + attrName;
        return false;
    }

    int power = j.value("power", 0);
    int maxPP = j.value("maxPP", 20);
    bool deletable = j.value("deletable", true);
    int priority = j.value("priority", 8);
    std::string scripterPath = j.value("scripterPath", "");
    bool guaranteedHit = j.value("guaranteedHit", false);

    skills.emplace_back(std::make_shared<SkillBase>(id, std::move(name), std::move(description), type, attr, power,
                                                    maxPP, deletable, priority, std::move(scripterPath),
                                                    guaranteedHit));
    return true;
}
