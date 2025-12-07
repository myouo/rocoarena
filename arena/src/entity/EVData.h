#pragma once

struct EVData {
    int eEne = 0, eAtk = 0, eDef = 0, eSpA = 0, eSpD = 0, eSpe = 0;
    EVData(int eEne = 0, int eAtk = 0, int eDef = 0, int eSpA = 0, int eSpD = 0, int eSpe = 0) :
        eEne(eEne), eAtk(eAtk), eDef(eDef), eSpA(eSpA), eSpD(eSpD), eSpe(eSpe) {};
};
