#pragma once
#include "Pet.h"

/*
    宠物的真实数值归于宠物个体而非类别，因此将函数作为Pet的成员函数。
    attr好像有点重复了，改个名字
    就Stat吧！
*/
void Pet::calcRealStat(BS bs, IVData ivs, EVData evs, NatureType ntype, int level = 100) {
    //数组以索引
    std::array<int,6> base = {bs.bEne, bs.bAtk, bs.bDef, bs.bSpA, bs.bSpD, bs.bSpe};
    std::array<int,6> iv   = {ivs.iEne, ivs.iAtk, ivs.iDef, ivs.iSpA, ivs.iSpD, ivs.iSpe};
    std::array<int,6> ev   = {evs.eEne, evs.eAtk, evs.eDef, evs.eSpA, evs.eSpD, evs.eSpe};
    std::array<int*,6> r   = {&rs.rEne, &rs.rAtk, &rs.rDef, &rs.rSpA, &rs.rSpD, &rs.rSpe};

    // hp
    *r[Ene] = int((base[Ene] * 2 + iv[Ene]) * level / 100.0) + level + 10 + (ev[Ene] >> 2);

    // 其他
    for (int i = 1; i < 6; ++i)
        *r[i] = int((base[i] * 2 + iv[i]) * level / 100.0) + 5 + (ev[i] >> 2);

    // 性格修正
    auto it = NATURE_TABLE.find(ntype);
    const Nature& nature = (it != NATURE_TABLE.end()) ? it->second : NEUTRAL_NATURE;

    if (nature.up != nature.down) {
        *r[nature.up] = int(std::round(*r[nature.up] * 1.1));
        *r[nature.down] = int(std::round(*r[nature.down] * 0.9));
    }
}