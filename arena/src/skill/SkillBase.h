#pragma once

#include <memory>
#include <string>
#include <utility>

#include <entity/Attr.h>

enum class SkillType { Physical = 0, Magical = 1, Status = 2 };

inline const std::string SkillTypeTable[] = { "Physical", "Magical", "Status" };

class SkillBase {
  public:
    // construct
    SkillBase(int id, std::string name, std::string description, SkillType type, AttrType attr, int power = 0,
              int maxPP = 20, bool deletable = true, int priority = 8, std::string scripterPath = {},
              bool guaranteedHit = false)
        : _id(id), _name(std::make_shared<std::string>(std::move(name))),
          _description(std::make_shared<std::string>(std::move(description))), _skilltype(type), _attr(attr),
          _power(power), _maxPP(maxPP), _currentPP(maxPP), _deletable(deletable), _priority(priority),
          _guaranteedHit(guaranteedHit), _scripterPath(std::move(scripterPath)) {}

    // construct with shared strings (for pooling/reuse)
    SkillBase(int id, std::shared_ptr<std::string> name, std::shared_ptr<std::string> description, SkillType type,
              AttrType attr, int power, int maxPP, bool deletable, int priority, std::string scripterPath,
              bool guaranteedHit)
        : _id(id), _name(std::move(name)), _description(std::move(description)), _skilltype(type), _attr(attr),
          _power(power), _maxPP(maxPP), _currentPP(maxPP), _deletable(deletable), _priority(priority),
          _guaranteedHit(guaranteedHit), _scripterPath(std::move(scripterPath)) {}

    // get function
    int id() const { return _id; }
    const std::string& name() const { return *_name; }
    const std::string& info() const { return *_description; }
    SkillType skillType() const { return _skilltype; }
    AttrType skillAttr() const { return _attr; }
    int skillPower() const { return _power; }
    int skillMaxPP() const { return _maxPP; }
    bool skillDeletable() const { return _deletable; }
    int skillPriority() const { return _priority; }
    const std::string& skillScripterPath() const { return _scripterPath; }
    bool isGuaranteedHit() const { return _guaranteedHit; }
    int currentPP() const { return _currentPP; }

  private:
    int _id;
    std::shared_ptr<std::string> _name;
    std::shared_ptr<std::string> _description;
    SkillType _skilltype;
    AttrType _attr;
    int _power = 0;
    int _maxPP = 20;
    int _currentPP;
    bool _deletable = true;
    int _priority = 8;
    bool _guaranteedHit = false;
    // int _accuracy = 80;
    std::string _scripterPath;
};
