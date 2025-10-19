#pragma once

#include "../entity/Pet.h"
#include "../entity/Skill.h"

class BattleSystem {
public:
    void init(Pet& p1, Pet& p2);
    void takeTurn();
};