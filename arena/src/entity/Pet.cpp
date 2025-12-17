#include "Pet.h"

#include <algorithm>

#include <skill/SkillRegistry.h>

/*
    宠物的真实数值归于宠物个体而非类别，因此将函数作为Pet的成员函数。
    attr好像有点重复了，改个名字
    就Stat吧！
*/
void Pet::calcRealStat(BS bs, IVData ivs, EVData evs, NatureType ntype, int levelValue) {
    level_ = levelValue;
    const int level = level_;
    //数组以索引
    std::array<int, 6> base = { bs.bEne, bs.bAtk, bs.bDef, bs.bSpA, bs.bSpD, bs.bSpe };
    std::array<int, 6> iv = { ivs.iEne, ivs.iAtk, ivs.iDef, ivs.iSpA, ivs.iSpD, ivs.iSpe };
    std::array<int, 6> ev = { evs.eEne, evs.eAtk, evs.eDef, evs.eSpA, evs.eSpD, evs.eSpe };
    std::array<int*, 6> r = { &rs.rEne, &rs.rAtk, &rs.rDef, &rs.rSpA, &rs.rSpD, &rs.rSpe };

    // hp
    *r[Ene] = int((base[Ene] * 2 + iv[Ene]) * level / 100.0) + level + 10 + (ev[Ene] >> 2);

    // 其他
    for (int i = 1; i < 6; ++i) *r[i] = int((base[i] * 2 + iv[i]) * level / 100.0) + 5 + (ev[i] >> 2);

    // 性格修正
    auto it = NATURE_TABLE.find(ntype);
    const Nature& nature = (it != NATURE_TABLE.end()) ? it->second : NEUTRAL_NATURE;

    if (nature.up != nature.down) {
        //此处不可以用std::round，因为洛克王国的计算方法是向下截断而非四舍五入
        *r[nature.up] = int((*r[nature.up] * 1.1));
        *r[nature.down] = int((*r[nature.down] * 0.9));
    }

    currentHP_ = rs.rEne;
}

void Pet::setLearnableSkills(std::vector<int> skills) {
    learnableSkillIds_ = std::move(skills);
}

bool Pet::canLearn(int skillId) const {
    return std::find(learnableSkillIds_.begin(), learnableSkillIds_.end(), skillId) != learnableSkillIds_.end();
}

Pet::SkillSlot* Pet::findSlotById(int skillId) {
    for (auto& slot : learnedSkills_) {
        if (slot.has_value() && slot->base && slot->base->id() == skillId) {
            return &slot.value();
        }
    }
    return nullptr;
}

std::optional<std::size_t> Pet::firstEmptySlot() {
    for (std::size_t i = 0; i < learnedSkills_.size(); ++i) {
        if (!learnedSkills_[i].has_value()) return i;
    }
    return std::nullopt;
}

bool Pet::configureSkill(int skillId, const SkillRegistry& registry, int replaceSlotIndex) {
    if (!canLearn(skillId)) return false;
    const SkillBase* base = registry.get(skillId);
    if (!base) return false;

    // If already learned, refresh PP and base pointer.
    if (SkillSlot* existing = findSlotById(skillId)) {
        existing->base = base;
        existing->currentPP = base->skillMaxPP();
        return true;
    }

    if (auto empty = firstEmptySlot()) {
        learnedSkills_[*empty] = SkillSlot{ base, base->skillMaxPP() };
        return true;
    }

    if (replaceSlotIndex < 0 || static_cast<std::size_t>(replaceSlotIndex) >= kMaxSkillSlots) {
        return false; // Full and no valid replacement index.
    }

    learnedSkills_[static_cast<std::size_t>(replaceSlotIndex)] = SkillSlot{ base, base->skillMaxPP() };
    return true;
}

bool Pet::consumePP(int skillId, int amount) {
    if (amount <= 0) return true;

    // Double-loss mark adds 1 extra PP consumption per use.
    if (buff_.hasDoubleLoss()) {
        amount += 1;
    }

    SkillSlot* slot = findSlotById(skillId);
    if (!slot) return false;
    if (slot->currentPP < amount) return false;
    slot->currentPP -= amount;
    return true;
}

int Pet::takeDamage(int amount) {
    if (amount <= 0) return currentHP_;
    currentHP_ -= amount;
    if (currentHP_ < 0) currentHP_ = 0;
    return currentHP_;
}

void Pet::restoreHP(int amount) {
    if (amount <= 0) return;
    currentHP_ += amount;
    const int maxHp = maxHP();
    if (currentHP_ > maxHp) currentHP_ = maxHp;
}
