#pragma once

struct IVData {
    int iEne = 0, iAtk = 0, iDef = 0, iSpA = 0, iSpD = 0, iSpe = 0;
    IVData(int iEne = 0, int iAtk = 0, int iDef = 0, int iSpA = 0, int iSpD = 0, int iSpe = 0) :
        iEne(iEne), iAtk(iAtk), iDef(iDef), iSpA(iSpA), iSpD(iSpD), iSpe(iSpe) {};
};
