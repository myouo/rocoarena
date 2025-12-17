

#include "SkillRegistry.h"

#include <filesystem>
#include <fstream>
#include <memory>
#include <sstream>
#include <utility>

#include <nlohmann/json.hpp>

using nlohmann::json;

namespace {
bool isValidSkillType(int value) {
    switch (static_cast<SkillType>(value)) {
        case SkillType::Physical:
        case SkillType::Magical:
        case SkillType::Status:
            return true;
        default:
            return false;
    }
}
} // namespace

bool SkillRegistry::load(std::vector<SkillBase> skills, std::string* error) {
    std::vector<std::shared_ptr<SkillBase>> sharedSkills;
    sharedSkills.reserve(skills.size());
    for (auto& s : skills) {
        sharedSkills.emplace_back(std::make_shared<SkillBase>(std::move(s)));
    }
    return loadShared(std::move(sharedSkills), error);
}

bool SkillRegistry::loadShared(std::vector<std::shared_ptr<SkillBase>> skills, std::string* error) {
    skills_.clear();

    for (auto& skill : skills) {
        std::string err;
        if (!skill || !validate(*skill, err)) {
            if (error) *error = err.empty() ? "invalid skill" : err;
            skills_.clear();
            return false;
        }

        if (skills_.count(skill->id())) {
            std::ostringstream oss;
            oss << "duplicate skill id: " << skill->id();
            if (error) *error = oss.str();
            skills_.clear();
            return false;
        }

        skills_.emplace(skill->id(), std::move(skill));
    }

    return true;
}

const SkillBase* SkillRegistry::get(int id) const {
    auto it = skills_.find(id);
    if (it == skills_.end()) return nullptr;
    return it->second.get();
}

bool SkillRegistry::loadFromJson(const std::string& path, std::string* error) {
    std::ifstream in(path);
    if (!in.is_open()) {
        if (error) *error = "failed to open file: " + path;
        return false;
    }

    json data;
    try {
        in >> data;
    } catch (const std::exception& e) {
        if (error) *error = std::string("invalid json: ") + e.what();
        return false;
    }

    std::vector<std::shared_ptr<SkillBase>> skills;
    if (data.is_array()) {
        for (const auto& item : data) {
            if (!parseSkillJsonShared(item, skills, error)) return false;
        }
    } else if (data.is_object()) {
        if (!parseSkillJsonShared(data, skills, error)) return false;
    } else {
        if (error) *error = "skill json root must be array or object";
        return false;
    }

    return loadShared(std::move(skills), error);
}

bool SkillRegistry::loadFromDirectory(const std::string& dir, std::string* error) {
    namespace fs = std::filesystem;
    std::vector<std::shared_ptr<SkillBase>> skills;
    fs::path root(dir);

    std::error_code ec;
    if (!fs::exists(root, ec) || !fs::is_directory(root, ec)) {
        if (error) *error = "directory not found: " + dir;
        return false;
    }

    for (const auto& entry : fs::directory_iterator(root, ec)) {
        if (ec) break;
        if (!entry.is_regular_file()) continue;
        if (entry.path().extension() != ".json") continue;

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
                if (!parseSkillJsonShared(item, skills, error)) {
                    addContextError(error ? *error : "parse error");
                    return false;
                }
            }
        } else if (data.is_object()) {
            if (!parseSkillJsonShared(data, skills, error)) {
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

    if (skills.empty()) {
        if (error) *error = "no skill json files found in directory: " + dir;
        return false;
    }

    return loadShared(std::move(skills), error);
}

bool SkillRegistry::validate(const SkillBase& skill, std::string& err) const {
    if (skill.id() <= 0) {
        err = "id must be positive";
        return false;
    }
    if (skill.name().empty()) {
        err = "name is empty for id " + std::to_string(skill.id());
        return false;
    }
    if (skill.info().empty()) {
        err = "description is empty for id " + std::to_string(skill.id());
        return false;
    }
    if (skill.skillMaxPP() <= 0) {
        err = "maxPP must be > 0 for id " + std::to_string(skill.id());
        return false;
    }
    if (skill.skillPower() < 0) {
        err = "power must be >= 0 for id " + std::to_string(skill.id());
        return false;
    }
    if (skill.skillPriority() < 0 || skill.skillPriority() > 12) {
        err = "priority out of range for id " + std::to_string(skill.id());
        return false;
    }
    if (skill.skillAttr() == AttrType::None) {
        err = "attr is None for id " + std::to_string(skill.id());
        return false;
    }
    return true;
}

AttrType SkillRegistry::parseAttr(const std::string& attrName) const {
    if (auto it = AttrFromEN.find(attrName); it != AttrFromEN.end()) {
        return it->second;
    }
    if (auto it = AttrFromZH.find(attrName); it != AttrFromZH.end()) {
        return it->second;
    }
    // if (attrName == "None") return AttrType::None;
    return AttrType::None;
}

bool SkillRegistry::parseSkillJson(const json& j, std::vector<SkillBase>& skills, std::string* error) const {
    if (!j.contains("id") || !j.contains("name") || !j.contains("description") || !j.contains("skillType")) {
        if (error) *error = "skill json missing required fields (id/name/description/skillType)";
        return false;
    }

    int id = j.value("id", 0);
    std::string name = j.value("name", "");
    std::string description = j.value("description", "");

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

    skills.emplace_back(id, std::move(name), std::move(description), type, attr, power, maxPP, deletable, priority,
                        std::move(scripterPath), guaranteedHit);
    return true;
}

bool SkillRegistry::parseSkillJsonShared(const json& j, std::vector<std::shared_ptr<SkillBase>>& skills,
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
