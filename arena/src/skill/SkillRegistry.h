#pragma once

#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <robin_hood.h>

#include "SkillBase.h"

// Registry for loading and querying all skills by id.
class SkillRegistry {
  public:
    bool load(std::vector<SkillBase> skills, std::string* error = nullptr);
    bool loadShared(std::vector<std::shared_ptr<SkillBase>> skills, std::string* error = nullptr);

    bool loadFromJson(const std::string& path, std::string* error = nullptr);
    bool loadFromDirectory(const std::string& dir, std::string* error = nullptr);

    const SkillBase* get(int id) const;
    bool contains(int id) const { return skills_.find(id) != skills_.end(); }
    size_t size() const { return skills_.size(); }

  private:
    bool validate(const SkillBase& skill, std::string& err) const;
    AttrType parseAttr(const std::string& attrName) const;
    bool parseSkillJson(const nlohmann::json& j, std::vector<SkillBase>& skills, std::string* error) const;
    bool parseSkillJsonShared(const nlohmann::json& j, std::vector<std::shared_ptr<SkillBase>>& skills,
                              std::string* error) const;

    robin_hood::unordered_map<int, std::shared_ptr<SkillBase>> skills_;
};
