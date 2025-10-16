#pragma once
#include <string>
#include <cmath>

#include "Stat.h"
#include "BasicStatus.h"
#include "RealStatus.h"
#include "Nature.h"
#include "IVData.h"
#include "EVData.h"
#include "Attr.h"

class Species{
public:
    //编号
    int No;
    //宠物名称
    std::string name;
    //宠物属性
    Attr attr;
    //宠物种族值
    BS bs;
    //等级
    int level = 100;
    //性格
    Nature nature;
    //天赋
    IVData ivs;
    //努力值
    EVData evs;

    //计算出的实际数值
    RS rs;



    //种族值和性格计算
    void calcRealAttr(BS bs, IVData ivs, EVData evs, NatureType ntype, int level = 100);
};
