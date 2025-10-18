#pragma once
#include <string>
#include <cmath>
#include <memory>
#include <vector>
#include "nlohmann/json.hpp"
#include <optional>


#include "Stat.h"
#include "RealStatus.h"
#include "Nature.h"
#include "IVData.h"
#include "EVData.h"
#include "Attr.h"


typedef struct BasicStatus{
    int bEne, bAtk, bDef, bSpA, bSpD, bSpe;
    BasicStatus(int bEne, int bAtk, int bDef, int bSpA, int bSpD, int bSpe) : 
        bEne(bEne), bAtk(bAtk), bDef(bDef), bSpA(bSpA), bSpD(bSpD), bSpe(bSpe) {};
} BS;


class Species{
public:
    using Ptr = std::shared_ptr<const Species>;

    static Ptr createFromJson(const nlohmann::json& j);

    const int id() const noexcept {return id_;}
    const std::string& name() const noexcept {return name_;}
    const std::array<AttrType, 2>& attrs() const noexcept {return attrs_;}
    Species(int id, std::string name, std::array<AttrType, 2> attrs, BS bs) : id_(id), name_(name), attrs_(attrs), bs_(bs) {};

private:
    //编号
    int id_;
    //宠物名称
    std::string name_;
    //宠物属性
    std::array<AttrType, 2> attrs_ = {AttrType::None, AttrType::None};
    //宠物种族值
    BS bs_;
};
