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
    {"dWater", AttrType::dGrass},
    {"dGrass", AttrType::dWater}
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
    {"神草", AttrType::dGrass},
    {"神水", AttrType::dWater}
};

static const std::string Attr2StringEN[] = {
    "Normal", "Fire", "Water", "Electric", "Grass",
    "Ice", "Fighting", "Poison", "Ground", "Flying",
    "Cute", "Worm", "Rock", "Ghost", "Dragon", "Demon",
    "Steel", "Light", "dFire", "dGrass", "dWater"
};

enum AttrAdvantage {
    Neutral = 0,
    Counter = 1,
    Resist = -1
};


class AttrChart {
public:
    static constexpr int N = static_cast<int>(AttrType::COUNT);
    static constexpr std::array<std::array<AttrAdvantage,N>,N> attrChart = {{
        //普通 火系 水系 电系 草系 冰系 武系 毒系 土系 翼系 萌系 虫系 石系 幽灵 龙系 恶魔 机械 光系 神火 神草 神水
        //Normal
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Fire
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist},
        //Water
        {AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral},
        //Electric
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral},
        //Grass
        {AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral},
        //Ice
        {AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist},
        //Fighting
        {AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Poison
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Ground
        {AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral},
        //Flying
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Cute
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Worm
        {AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Rock
        {AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Ghost
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Dragon
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Demon
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //Steel
        {AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Resist},
        //Light
        {AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral, AttrAdvantage::Neutral},
        //dFire
        {AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Neutral, AttrAdvantage::Counter, AttrAdvantage::Resist},
        //dGrass
        {AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral, AttrAdvantage::Counter},
        //dWater
        {AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Counter, AttrAdvantage::Resist, AttrAdvantage::Neutral}
    }};
};