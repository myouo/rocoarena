#pragma once

#include "Attr.h"

struct Skill {
    std::string name;
    AttrType attr;
    int power;
    int pp;
    int maxPP;
    int priority;
    bool isSpecial;
    double accuracy;
};
