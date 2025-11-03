#pragma once
#include <string>

#include "Species.h"

class Pet{
public:
    //种族值和性格计算
    void calcRealStat(BS bs, IVData ivs, EVData evs, NatureType ntype, int level = 100);
    RS getRS() {return rs;}
    Pet(Species* sp, IVData iv, EVData ev) : species(sp), ivs(iv), evs(ev) {};
private:
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