#pragma once
#include <string>

#include "Species.h"

class Pet{
    //模板
    const Species* species;
    //天赋
    IVData ivs;
    //努力值
    EVData evs;
    //等级
    int level = 100;
    //性格
    Nature nature;
    //计算出的实际数值
    RS rs;
};