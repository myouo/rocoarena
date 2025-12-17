#pragma once

#include <array>
#include <cstdint>
#include <cstddef>

#include <entity/Attr.h>
#include <entity/Stat.h>
#include <forward.h>

// 13种异常状态
enum class Ailment : std::uint8_t {
    None = 0,
    Trapped,   // 束缚
    DeepSleep, // 沉睡（特殊睡眠）
    Bewitch,   // 迷惑
    Curse,     // 诅咒
    Parasite,  // 寄生
    Fear,      // 恐惧
    Confusion, // 混乱
    Toxic,     // 剧毒
    Poison,    // 中毒
    Freeze,    // 冰冻
    Burn,      // 烧伤
    Paralysis, // 麻醉
    Sleep,     // 睡眠
    Count      // 计数哨兵，不代表实际状态
};

// 针对异常/能力降低的免疫配置，可由技能/组件临时提供。
struct ImmunityProfile {
    // 静态免疫列表：拥有该技能/组件时自带。
    std::array<bool, static_cast<std::size_t>(Ailment::Count)> ailmentImmune{};
    // 是否完全无视异常（技能开启时的“免疫所有异常”）
    bool ignoreAllAilments = false;
    // 是否无视能力降低（技能开启时的“免疫负面异常”）
    bool ignoreNegativeStages = false;

    bool immuneTo(Ailment status) const;
    static ImmunityProfile fromAttrs(const std::array<AttrType, 2>& attrs);
};

struct ControlTurnResult {
    bool skipAction = false; // 控制类异常，本回合禁止主动行动（睡眠/恐惧/冻结/麻醉/迷惑等）
};

// 记录战斗时宠物的异常状态与能力等级。
class Buff {
  public:
    Buff();

    void reset();

    // —— 异常状态 —— //
    // 结合属性免疫 + 技能/组件免疫，忽略 respectImmunity 的旧语义。
    bool applyAilment(Ailment status, const std::array<AttrType, 2>& attrs, const ImmunityProfile& immunity = {});
    // 便捷接口：施加时处理即时效果（诅咒返伤、烧伤/麻醉首回合强化变更等）。
    bool applyAilmentWithEffects(Ailment status, const std::array<AttrType, 2>& attrs, Pet& self,
                                 Pet* opponent = nullptr, const ImmunityProfile& immunity = {});
    // 控制异常与非控制异常
    Ailment primaryAilment() const { return primary_; } // 互斥组：混乱/冰冻/恐惧/睡眠/沉睡/迷惑/麻醉/烧伤
    Ailment secondaryAilment() const { return secondary_; } // 互斥组：中毒/剧毒/诅咒/寄生
    bool isTrapped() const { return trapped_; }
    bool hasAilment(Ailment status) const;
    void clearPrimary();
    void clearSecondary();
    void clearTrapped();
    void clearAilments();
    // 回合开始时处理控制类异常（概率解除/回合数等），返回本回合是否禁止行动。
    ControlTurnResult onTurnStart();
    // 作用目标为“敌方”时，是否因混乱改为“自身”（50%）。
    bool shouldRedirectConfusion() const;
    // 受到威力伤害时的异常处理（解除睡眠/沉睡/冰冻等）。
    void onPowerDamageTaken(const std::array<AttrType, 2>& attackerAttrs, bool isPowerDamage = true);
    // 回合结束时处理非控制异常（固伤/治疗/计数清除等）。
    void onEndTurnNonControl(Pet& self, Pet* opponent = nullptr);
    // 换下时需要的状态维护（如诅咒解除、剧毒层数重置）。
    void onSwitchOut();

    static bool isControl(Ailment status);    // 控制异常
    static bool isNonControl(Ailment status); // 非控制异常
    static bool isSleepGroup(Ailment status); // 沉睡/睡眠判定

    // —— 其他标记 —— //
    // 双损
    bool hasDoubleLoss() const { return doubleLoss_; }
    void setDoubleLoss(bool enable = true) { doubleLoss_ = enable; }
    void clearDoubleLoss() { doubleLoss_ = false; }

    // 防踢
    bool immuneToExpel() const { return immuneExpel_; }
    void setImmuneToExpel(bool enable = true) { immuneExpel_ = enable; }
    void clearImmuneToExpel() { immuneExpel_ = false; }

    // 🔒强
    bool stageLocked() const { return stageLocked_; }
    void setStageLocked(bool enable = true) { stageLocked_ = enable; }
    void clearStageLocked() { stageLocked_ = false; }

    // —— 能力等级（-6 ~ +6） —— //
    int stage(Stat stat) const { return statStages_[static_cast<std::size_t>(stat)]; }
    int applyStageToStat(Stat stat, int base) const;
    int changeStage(Stat stat, int delta, const ImmunityProfile* immunity = nullptr);
    void resetStages();

  private:
    static constexpr int kMinStage = -6;
    static constexpr int kMaxStage = 6;

    static bool isPrimaryGroup(Ailment status);
    static bool isSecondaryGroup(Ailment status);

    void resetCounter(Ailment status);
    int& counter(Ailment status);
    const int& counter(Ailment status) const;
    bool roll(double probability) const;
    void applyParalysisSpeedDrop();
    void applyBurnAttackDrop();
    void forceStageDelta(Stat stat, int delta);

    int clampStage(int value) const;

    Ailment primary_;
    Ailment secondary_;
    bool trapped_;
    bool doubleLoss_;
    bool immuneExpel_;
    bool stageLocked_;
    std::array<int, static_cast<std::size_t>(Ailment::Count)> ailmentTurns_;
    int toxicStacks_ = 0;
    std::array<int, kStatCount> statStages_;
};
