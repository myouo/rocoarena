#pragma once
#include <array>
#include <optional>
#include <string>
#include <vector>

#include <battle/Buff.h>

#include "Species.h"

class SkillBase;
class SkillRegistry;

class Pet {
  public:
    struct SkillSlot {
        const SkillBase* base = nullptr;
        int currentPP = 0;
    };
    static constexpr std::size_t kMaxSkillSlots = 4;

    //构造与析构
    Pet(Species* sp, IVData iv, EVData ev) : species(sp), ivs(iv), evs(ev){};
    ~Pet() = default;

    //种族值和性格计算
    void calcRealStat(BS bs, IVData ivs, EVData evs, NatureType ntype, int levelValue = 100);
    RS getRS() const { return rs; }
    int level() const { return level_; }
    int attack() const { return rs.rAtk; }
    int defense() const { return rs.rDef; }
    int specialAttack() const { return rs.rSpA; }
    int specialDefense() const { return rs.rSpD; }
    int stagedAttack() const { return buff_.applyStageToStat(Stat::Atk, rs.rAtk); }
    int stagedDefense() const { return buff_.applyStageToStat(Stat::Def, rs.rDef); }
    int stagedSpecialAttack() const { return buff_.applyStageToStat(Stat::SpA, rs.rSpA); }
    int stagedSpecialDefense() const { return buff_.applyStageToStat(Stat::SpD, rs.rSpD); }
    const std::array<AttrType, 2>& attrs() const { return species->attrs(); }
    Buff& buff() { return buff_; }
    const Buff& buff() const { return buff_; }

    // 属性与当前状态
    int currentHP() const { return currentHP_; }
    int maxHP() const { return rs.rEne; }
    int currentSpeed() const { return buff_.applyStageToStat(Stat::Spe, rs.rSpe); }
    bool isFainted() const { return currentHP_ <= 0; }

    // 技能相关
    const std::vector<int>& learnableSkills() const { return learnableSkillIds_; }
    void setLearnableSkills(std::vector<int> skills);
    bool canLearn(int skillId) const;
    // Add or replace a skill: if there is an empty slot, fill it; if full, replace the given slot.
    // When replacing, caller must provide a valid slot index (0-3).
    bool configureSkill(int skillId, const SkillRegistry& registry, int replaceSlotIndex = -1);
    bool consumePP(int skillId, int amount = 1);
    const std::array<std::optional<SkillSlot>, kMaxSkillSlots>& learnedSkills() const { return learnedSkills_; }

    // 战斗数值更新
    int takeDamage(int amount);
    void restoreHP(int amount);

  private:
    SkillSlot* findSlotById(int skillId);
    std::optional<std::size_t> firstEmptySlot();

    //模板
    const Species* species;
    //天赋
    IVData ivs;
    //努力值
    EVData evs;
    //等级
    int level_ = 100;
    //性格
    Nature nature{};
    //计算出的实际数值
    RS rs{};
    // 当前血量
    int currentHP_ = 0;
    //战斗状态
    Buff buff_{};

    // 技能
    std::vector<int> learnableSkillIds_{};
    std::array<std::optional<SkillSlot>, kMaxSkillSlots> learnedSkills_{};
};
