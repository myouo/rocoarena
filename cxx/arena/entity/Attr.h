#pragma once

#include <unordered_map>
#include <array>

enum class AttrType {
    Normal,
    Fire,
    Water,
    Electric,
    Grass,
    Ice,
    Fighting,
    Poison,
    Ground,
    Flying,
    Cute,
    Worm,
    Rock,
    Ghost,
    Dragon,
    Demon,
    Steel,
    Light,
    dFire,
    dWater,
    dGrass,
    COUNT
};

static std::unordered_map<std::string, AttrType> AttrFromEN = {
    {"Normal", AttrType::Normal},
    {"Fire", AttrType::Fire},
    {"Water", AttrType::Water},
    {"Electic", AttrType::Electric},
    {"Grass", AttrType::Grass},
    {"Ice", AttrType::Ice},
    {"Fighting", AttrType::Fighting},
    {"Poison", AttrType::Poison},
    {"Ground", AttrType::Ground},
    {"Flying", AttrType::Flying},
    {"Cute", AttrType::Cute},
    {"Worm", AttrType::Worm},
    {"Rock", AttrType::Rock},
    {"Ghost", AttrType::Ghost},
    {"Dragon", AttrType::Dragon},
    {"Demon", AttrType::Demon},
    {"Steel", AttrType::Steel},
    {"Light", AttrType::Light},
    {"dFire", AttrType::dFire},
    {"dWater", AttrType::dWater},
    {"dGrass", AttrType::dGrass}
};

static std::unordered_map<std::string, AttrType> AttrFromZH = {
    {"普通", AttrType::Normal},
    {"火", AttrType::Fire},
    {"水", AttrType::Water},
    {"电", AttrType::Electric},
    {"草", AttrType::Grass},
    {"冰", AttrType::Ice},
    {"武", AttrType::Fighting},
    {"毒", AttrType::Poison},
    {"土", AttrType::Ground},
    {"翼", AttrType::Flying},
    {"萌", AttrType::Cute},
    {"虫", AttrType::Worm},
    {"石", AttrType::Rock},
    {"幽", AttrType::Ghost},
    {"龙", AttrType::Dragon},
    {"恶魔", AttrType::Demon},
    {"机械", AttrType::Steel},
    {"光", AttrType::Light},
    {"神火", AttrType::dFire},
    {"神水", AttrType::dWater},
    {"神草", AttrType::dGrass}
};

static const std::string Attr2StringEN[] = {
    "Normal", "Fire", "Water", "Electric", "Grass",
    "Ice", "Fighting", "Poison", "Ground", "Flying",
    "Cute", "Worm", "Rock", "Ghost", "Dragon", "Demon",
    "Steel", "Light", "dFire", "dWater", "dGrass"
};

class AttrChart {
public:
    static constexpr int N = static_cast<int>(AttrType::COUNT);
    static constexpr std::array<std::array<double,N>,N> attrChart = {
        
    };
};