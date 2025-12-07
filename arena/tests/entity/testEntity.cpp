
#include <iostream>

#include <Pet.h>

int main() {
    
    BS b1{150, 30, 126, 140, 125, 120};
    IVData i1{31, 31, 31, 31, 31, 31};
    EVData e1{0, 36, 0, 252, 0, 0};
    Species* sp1 = new Species(1, "testCute", std::array<AttrType, 2>{AttrType::Cute, AttrType::None}, b1);
    Pet p1(sp1, i1, e1);
    p1.calcRealStat(b1, i1, e1, NatureType::Modest, 100);
    RS res = p1.getRS();

    printf("HP is %d\n", res.rEne);
    printf("Atk is %d\n", res.rAtk);
    printf("Def is %d\n", res.rDef);
    printf("SpA is %d\n", res.rSpA);
    printf("SpD is %d\n", res.rSpD);
    printf("Spe is %d\n", res.rSpe);

    auto res2 = AttrChart::getAttrAdvantage(AttrType::Water, std::array<AttrType,2>{AttrType::Rock, AttrType::Ground});
    auto res3 = AttrChart::getAttrAdvantage(AttrType::Fire, std::array<AttrType,2>{AttrType::Ice, AttrType::Worm});
    
    printf("the result2 is : %lf\n", res2);
    printf("the result3 is : %lf\n", res3);
    std::cout << "Attr 2 string :" << Attr2StringEN[(int)AttrType::dFire] << "\n";

    delete sp1;
    return 0;
}