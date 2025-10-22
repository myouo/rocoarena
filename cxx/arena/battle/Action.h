#pragma once

#include "../utils/Forward.h"

enum class ActionType {
    Skill,
    Switch,
    Potion,
    Flee
};

class Action {
public:
    Action(ActionType type) : type_(type) {};
    virtual ~Action() = default;

    ActionType getType() const {return type_;}
    
    virtual void execute(BattleSystem& battle, Pet& self, Pet& opponent) = 0;
private:
    ActionType type_;
};
