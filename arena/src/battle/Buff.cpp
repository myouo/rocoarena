#include "Buff.h"

#include <algorithm>
#include <cmath>

#include <Pet.h>
#include <rng/rng.h>

namespace {
bool hasAttr(const std::array<AttrType, 2>& attrs, AttrType target) {
    return attrs[0] == target || attrs[1] == target;
}
} // namespace

bool ImmunityProfile::immuneTo(Ailment status) const {
    const auto idx = static_cast<std::size_t>(status);
    if (idx >= ailmentImmune.size()) return false;
    return ailmentImmune[idx];
}

ImmunityProfile ImmunityProfile::fromAttrs(const std::array<AttrType, 2>& attrs) {
    ImmunityProfile p;
    auto grant = [&](Ailment ailment) { p.ailmentImmune[static_cast<std::size_t>(ailment)] = true; };
    if (hasAttr(attrs, AttrType::Ice)) {
        grant(Ailment::Freeze);
    }
    if (hasAttr(attrs, AttrType::Fire) || hasAttr(attrs, AttrType::dFire)) {
        grant(Ailment::Burn);
    }
    if (hasAttr(attrs, AttrType::Grass) || hasAttr(attrs, AttrType::dGrass)) {
        grant(Ailment::Parasite);
    }
    if (hasAttr(attrs, AttrType::Poison) || hasAttr(attrs, AttrType::Steel)) {
        grant(Ailment::Poison);
        grant(Ailment::Toxic);
    }
    return p;
}

Buff::Buff() {
    reset();
}

void Buff::reset() {
    primary_ = Ailment::None;
    secondary_ = Ailment::None;
    trapped_ = false;
    doubleLoss_ = false;
    immuneExpel_ = false;
    stageLocked_ = false;
    toxicStacks_ = 0;
    ailmentTurns_.fill(0);
    statStages_.fill(0);
}

bool Buff::applyAilment(Ailment status, const std::array<AttrType, 2>& attrs, const ImmunityProfile& immunity) {
    if (status == Ailment::None) {
        return false;
    }

    if (status == Ailment::Trapped) {
        const bool changed = !trapped_;
        trapped_ = true;
        return changed;
    }

    // 开启“完全无视异常”时，直接抵消新异常。
    if (immunity.ignoreAllAilments) {
        return false;
    }
    const auto attrImmunity = ImmunityProfile::fromAttrs(attrs);
    const bool immuneByAttr = attrImmunity.immuneTo(status);
    const bool immuneBySkill = immunity.immuneTo(status);

    if (immuneByAttr || immuneBySkill) {
        return false;
    }

    if (isPrimaryGroup(status)) {
        const bool changed = primary_ != status;
        if (changed) {
            resetCounter(primary_);
        }
        primary_ = status;
        resetCounter(status);
        if (status == Ailment::Paralysis && changed) {
            applyParalysisSpeedDrop();
        }
        if (status == Ailment::Burn && changed) {
            applyBurnAttackDrop();
        }
        return changed;
    }

    if (isSecondaryGroup(status)) {
        const bool changed = secondary_ != status;
        if (changed) {
            resetCounter(secondary_);
        }
        secondary_ = status;
        resetCounter(status);
        if (status == Ailment::Toxic) {
            toxicStacks_ = 0;
        }
        return changed;
    }

    return false;
}

bool Buff::applyAilmentWithEffects(Ailment status, const std::array<AttrType, 2>& attrs, Pet& self, Pet* opponent,
                                   const ImmunityProfile& immunity) {
    const bool changed = applyAilment(status, attrs, immunity);
    if (!changed) return false;

    if (status == Ailment::Curse && opponent) {
        const int dmg = std::max(1, self.currentHP() / 8);
        opponent->takeDamage(dmg);
    }

    return true;
}

bool Buff::hasAilment(Ailment status) const {
    switch (status) {
        case Ailment::None:
            return false;
        case Ailment::Trapped:
            return trapped_;
        default:
            if (isPrimaryGroup(status)) {
                return primary_ == status;
            }
            if (isSecondaryGroup(status)) {
                return secondary_ == status;
            }
            return false;
    }
}

void Buff::clearPrimary() {
    resetCounter(primary_);
    primary_ = Ailment::None;
}

void Buff::clearSecondary() {
    resetCounter(secondary_);
    secondary_ = Ailment::None;
}

void Buff::clearTrapped() {
    trapped_ = false;
}

void Buff::clearAilments() {
    clearPrimary();
    clearSecondary();
    clearTrapped();
    clearDoubleLoss();
    clearImmuneToExpel();
    clearStageLocked();
    ailmentTurns_.fill(0);
    toxicStacks_ = 0;
}

bool Buff::isControl(Ailment status) {
    switch (status) {
        case Ailment::Confusion:
        case Ailment::Freeze:
        case Ailment::Fear:
        case Ailment::Sleep:
        case Ailment::DeepSleep:
        case Ailment::Bewitch:
        case Ailment::Paralysis:
        case Ailment::Trapped:
            return true;
        default:
            return false;
    }
}

bool Buff::isNonControl(Ailment status) {
    switch (status) {
        case Ailment::Poison:
        case Ailment::Toxic:
        case Ailment::Curse:
        case Ailment::Parasite:
        case Ailment::Burn:
            return true;
        default:
            return false;
    }
}

bool Buff::isSleepGroup(Ailment status) {
    return status == Ailment::Sleep || status == Ailment::DeepSleep;
}

int Buff::changeStage(Stat stat, int delta, const ImmunityProfile* immunity) {
    // 锁定强化：比免疫负面更高优先级，正负变化都被拒绝。
    if (stageLocked_) {
        return 0;
    }
    if (delta < 0 && immunity && immunity->ignoreNegativeStages) {
        return 0;
    }
    const auto idx = static_cast<std::size_t>(stat);
    const int before = statStages_[idx];
    const int after = clampStage(before + delta);
    statStages_[idx] = after;
    return after - before;
}

int Buff::applyStageToStat(Stat stat, int base) const {
    const int currentStage = stage(stat);
    if (currentStage == 0 || base <= 0) {
        return base;
    }

    const double factor = static_cast<double>(std::abs(currentStage)) / 2.0 + 1.0;
    const double scaled = currentStage > 0 ? base * factor : base / factor;
    return static_cast<int>(scaled);
}

ControlTurnResult Buff::onTurnStart() {
    ControlTurnResult res;

    switch (primary_) {
        case Ailment::Sleep:
            counter(Ailment::Sleep)++;
            res.skipAction = true;
            if (counter(Ailment::Sleep) >= 1 && roll(0.2)) {
                clearPrimary();
            } else if (counter(Ailment::Sleep) >= 4) {
                clearPrimary();
            }
            break;
        case Ailment::DeepSleep:
            counter(Ailment::DeepSleep)++;
            res.skipAction = true;
            if (counter(Ailment::DeepSleep) >= 1 && roll(0.2)) {
                clearPrimary();
            } else if (counter(Ailment::DeepSleep) >= 4) {
                clearPrimary();
            }
            break;
        case Ailment::Fear:
            counter(Ailment::Fear)++;
            res.skipAction = true;
            if (counter(Ailment::Fear) >= 1 && (roll(0.4) || counter(Ailment::Fear) >= 4)) {
                clearPrimary();
            }
            break;
        case Ailment::Freeze:
            counter(Ailment::Freeze)++;
            res.skipAction = true;
            if (roll(0.2)) {
                clearPrimary();
            }
            break;
        case Ailment::Bewitch:
            counter(Ailment::Bewitch)++;
            if (counter(Ailment::Bewitch) >= 1 && roll(0.5)) {
                res.skipAction = true;
            }
            break;
        case Ailment::Paralysis:
            counter(Ailment::Paralysis)++;
            if (counter(Ailment::Paralysis) >= 1 && roll(0.5)) {
                res.skipAction = true;
            }
            break;
        case Ailment::Confusion:
            counter(Ailment::Confusion)++;
            if (counter(Ailment::Confusion) >= 1 && roll(0.4)) {
                clearPrimary();
            } else if (counter(Ailment::Confusion) >= 4) {
                clearPrimary();
            }
            break;
        default:
            break;
    }

    return res;
}

bool Buff::shouldRedirectConfusion() const {
    if (primary_ != Ailment::Confusion) return false;
    return roll(0.5);
}

void Buff::onPowerDamageTaken(const std::array<AttrType, 2>& attackerAttrs, bool isPowerDamage) {
    if (!isPowerDamage) {
        return;
    }

    const bool monoLight = attackerAttrs[0] == AttrType::Light && attackerAttrs[1] == AttrType::None;

    if (primary_ == Ailment::Sleep && !monoLight) {
        clearPrimary();
    }

    if (primary_ == Ailment::Freeze) {
        if (hasAttr(attackerAttrs, AttrType::Fire) || hasAttr(attackerAttrs, AttrType::dFire)) {
            clearPrimary();
        }
    }

    if (primary_ == Ailment::Burn) {
        if (hasAttr(attackerAttrs, AttrType::Water) || hasAttr(attackerAttrs, AttrType::dWater)) {
            clearPrimary();
        }
    }
}

void Buff::resetCounter(Ailment status) {
    if (status == Ailment::Toxic) {
        toxicStacks_ = 0;
    }
    counter(status) = 0;
}

int& Buff::counter(Ailment status) {
    return ailmentTurns_[static_cast<std::size_t>(status)];
}

const int& Buff::counter(Ailment status) const {
    return ailmentTurns_[static_cast<std::size_t>(status)];
}

bool Buff::roll(double probability) const {
    if (probability <= 0.0) return false;
    if (probability >= 1.0) return true;
    return RNG::instance().chance(probability);
}

void Buff::applyParalysisSpeedDrop() {
    const auto idx = static_cast<std::size_t>(Stat::Spe);
    statStages_[idx] = clampStage(statStages_[idx] - 1);
}

void Buff::applyBurnAttackDrop() {
    forceStageDelta(Stat::Atk, -2);
}

void Buff::forceStageDelta(Stat stat, int delta) {
    const auto idx = static_cast<std::size_t>(stat);
    statStages_[idx] = clampStage(statStages_[idx] + delta);
}

void Buff::onEndTurnNonControl(Pet& self, Pet* opponent) {
    auto damageCap = [](int dmg, int cap) { return (dmg > cap) ? cap : dmg; };
    const int maxHp = self.maxHP();

    if (secondary_ == Ailment::Curse) {
        const int dmg = damageCap(maxHp / 4, 500);
        self.takeDamage(dmg);
    }

    if (secondary_ == Ailment::Parasite) {
        counter(Ailment::Parasite)++;
        const int dmg = static_cast<int>(maxHp * 0.125);
        self.takeDamage(dmg);
        if (opponent) {
            opponent->restoreHP(dmg);
        }
        if (counter(Ailment::Parasite) >= 8) {
            clearSecondary();
        }
    }

    if (secondary_ == Ailment::Toxic) {
        toxicStacks_++;
        const double raw = maxHp * 0.0625 * static_cast<double>(toxicStacks_);
        const int dmg = damageCap(static_cast<int>(raw), 500);
        self.takeDamage(dmg);
    }

    if (secondary_ == Ailment::Poison) {
        const int dmg = damageCap(static_cast<int>(maxHp * 0.125), 200);
        self.takeDamage(dmg);
    }

    if (primary_ == Ailment::Burn) {
        const int dmg = damageCap(static_cast<int>(maxHp * 0.125), 200);
        self.takeDamage(dmg);
    }
}

void Buff::onSwitchOut() {
    toxicStacks_ = 0;
    if (secondary_ == Ailment::Curse) {
        clearSecondary();
    }
}

void Buff::resetStages() {
    statStages_.fill(0);
}

bool Buff::isPrimaryGroup(Ailment status) {
    switch (status) {
        case Ailment::Confusion:
        case Ailment::Freeze:
        case Ailment::Fear:
        case Ailment::Sleep:
        case Ailment::DeepSleep:
        case Ailment::Bewitch:
        case Ailment::Paralysis:
        case Ailment::Burn:
            return true;
        default:
            return false;
    }
}

bool Buff::isSecondaryGroup(Ailment status) {
    switch (status) {
        case Ailment::Poison:
        case Ailment::Toxic:
        case Ailment::Curse:
        case Ailment::Parasite:
            return true;
        default:
            return false;
    }
}

int Buff::clampStage(int value) const {
    if (value < kMinStage) {
        return kMinStage;
    }
    if (value > kMaxStage) {
        return kMaxStage;
    }
    return value;
}
