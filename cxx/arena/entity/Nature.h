#pragma once

#include "Stat.h"
#include <string>
#include <map>


struct Nature {
    Stat up;
    Stat down;
    std::string name;
};

enum class NatureType {
    Hardy, Lonely, Adamant, Naughty, Brave,   // 攻击+
    Bold, Docile, Impish, Lax, Relaxed,       // 防御+
    Modest, Mild, Bashful, Rash, Quiet,       // 特攻+
    Calm, Gentle, Careful, Quirky, Sassy,     // 特防+
    Timid, Hasty, Jolly, Naive, Serious       // 速度+
};

static const std::map<NatureType, Nature> NATURE_TABLE = {
    {NatureType::Lonely,  {Atk, Def, "孤僻 (Lonely)"}},
    {NatureType::Adamant, {Atk, SpA, "固执 (Adamant)"}},
    {NatureType::Naughty, {Atk, SpD, "调皮 (Naughty)"}},
    {NatureType::Brave,   {Atk, Spe, "勇敢 (Brave)"}},
    {NatureType::Hardy,   {Atk, Atk, "坦率 (Hardy)"}},

    {NatureType::Bold,    {Def, Atk, "大胆 (Bold)"}},
    {NatureType::Impish,  {Def, SpA, "淘气 (Impish)"}},
    {NatureType::Lax,     {Def, SpD, "无虑 (Lax)"}},
    {NatureType::Relaxed, {Def, Spe, "悠闲 (Relaxed)"}},
    {NatureType::Docile,  {Def, Def, "害羞 (Docile)"}},

    {NatureType::Modest,  {SpA, Atk, "保守 (Modest)"}},
    {NatureType::Mild,    {SpA, Def, "稳重 (Mild)"}},
    {NatureType::Rash,    {SpA, SpD, "马虎 (Rash)"}},
    {NatureType::Quiet,   {SpA, Spe, "冷静 (Quiet)"}},
    {NatureType::Bashful, {SpA, SpA, "认真 (Bashful)"}},

    {NatureType::Calm,    {SpD, Atk, "沉着 (Calm)"}},
    {NatureType::Gentle,  {SpD, Def, "温顺 (Gentle)"}},
    {NatureType::Careful, {SpD, SpA, "慎重 (Careful)"}},
    {NatureType::Sassy,   {SpD, Spe, "狂妄 (Sassy)"}},
    {NatureType::Quirky,  {SpD, SpD, "实干 (Quirky)"}},

    {NatureType::Timid,   {Spe, Atk, "胆小 (Timid)"}},
    {NatureType::Hasty,   {Spe, Def, "急躁 (Hasty)"}},
    {NatureType::Jolly,   {Spe, SpA, "开朗 (Jolly)"}},
    {NatureType::Naive,   {Spe, SpD, "天真 (Naive)"}},
    {NatureType::Serious, {Spe, Spe, "浮躁 (Serious)"}}
};

static const Nature NEUTRAL_NATURE = {Atk, Atk};