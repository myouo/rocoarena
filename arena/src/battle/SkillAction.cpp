#include "SkillAction.h"

#include <algorithm>
#include <exception>

#include <BattleSystem.h>
#include <Attr.h>
#include <logger/logger.h>
#include <rng/rng.h>
#include <scripter/scripter.h>
#include <Player.h>
#include <Stat.h>

namespace {
int calculatePowerDamage(const SkillBase& skill, const Pet& attacker, const Pet& defender) {
    if (skill.skillPower() <= 0) {
        return 0;
    }

    const bool isPhysical = skill.skillType() == SkillType::Physical;
    const int effectiveAttack = isPhysical ? attacker.stagedAttack() : attacker.stagedSpecialAttack();
    int effectiveDefense = isPhysical ? defender.stagedDefense() : defender.stagedSpecialDefense();
    if (effectiveDefense <= 0) {
        effectiveDefense = 1;
    }

    const double level = attacker.level();
    const int base = static_cast<int>(
        ((level * 0.4 + 2.0) * skill.skillPower() * effectiveAttack / effectiveDefense) / 50.0 + 2.0);
    const double attrModifier = AttrChart::getAttrAdvantage(skill.skillAttr(), defender.attrs());
    const int randFactor = RNG::instance().range<int>(217, 255);
    const double finalDamage = base * attrModifier * static_cast<double>(randFactor) / 255.0;
    const int roundedDamage = std::max(0, static_cast<int>(finalDamage));

    return std::min(roundedDamage, 99999);
}
} // namespace

void SkillAction::execute(BattleSystem& /*battle*/, Player& self, Player& opponent) {
    if (!skill_) {
        LOG_ERROR(module(), "SkillAction has no bound skill.");
        return;
    }

    Pet& selfPet = self.activePet();
    Pet& targetPet = opponent.activePet();

    LOG_INFO(module(), "Executing skill [", skill_->id(), "] ", skill_->name(), " | priority=", skill_->skillPriority(),
             " | guaranteedHit=", (guaranteedHit() ? "true" : "false"));

    // Consume PP before executing.
    if (!selfPet.consumePP(skill_->id())) {
        LOG_WARN(module(), "Skill PP is empty for skill id ", skill_->id());
        return;
    }

    // Lua scripting hook
    const std::string& scriptPath = skill_->skillScripterPath();
    if (!scriptPath.empty()) {
        Scripter scripter;
        scripter.registerFunction("deal_damage", [&](int amount) { targetPet.takeDamage(amount); });
        scripter.registerFunction("minus_pp", [&](int amount) { return selfPet.consumePP(skill_->id(), amount); });
        scripter.registerFunction("heal_self", [&](int amount) { selfPet.restoreHP(amount); });
        scripter.set("skill_power", skill_->skillPower());
        scripter.set("attacker_speed", selfPet.currentSpeed());

        if (!scripter.runScript(scriptPath)) {
            LOG_ERROR(module(), "Failed to run script: ", scriptPath);
            return;
        }
        try {
            scripter.call<void>("on_cast");
        } catch (const std::exception& e) {
            LOG_ERROR(module(), "Lua on_cast failed: ", e.what());
        }
        return;
    }

    // Fallback: fixed damage using skill power.
    const int damage = calculatePowerDamage(*skill_, selfPet, targetPet);
    targetPet.takeDamage(damage);
}
