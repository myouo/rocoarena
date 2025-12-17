#pragma once

#include <memory>
#include <string>
#include <vector>

#include <nlohmann/json.hpp>
#include <robin_hood.h>

#include "SkillBase.h"
#include "SkillRegistry.h"

class SkillPool {
  public:
    bool loadAllFromDirectory(const std::string& dir, std::string* error = nullptr);

    const std::shared_ptr<SkillBase> getShared(int id) const;
    bool contains(int id) const { return skills_.find(id) != skills_.end(); }
    size_t size() const { return skills_.size(); }

    bool buildSubsetRegistry(const std::vector<int>& ids, SkillRegistry& registry, std::string* error = nullptr) const;

  private:
    bool parseSkillJson(const nlohmann::json& j, std::vector<std::shared_ptr<SkillBase>>& skills,
                        std::string* error) const;
    AttrType parseAttr(const std::string& attrName) const;

    robin_hood::unordered_map<int, std::shared_ptr<SkillBase>> skills_;
};
