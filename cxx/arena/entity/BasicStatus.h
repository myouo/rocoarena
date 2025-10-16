#pragma once

typedef struct BasicStatus{
    int bEne, bAtk, bDef, bSpA, bSpD, bSpe;
    BasicStatus(int bEne, int bAtk, int bDef, int bSpA, int bSpD, int bSpe) : 
        bEne(bEne), bAtk(bAtk), bDef(bDef), bSpA(bSpA), bSpD(bSpD), bSpe(bSpe) {};
} BS;