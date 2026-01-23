#include "SkillAction.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <filesystem>
#include <optional>

#include <BattleSystem.h>
#include <Attr.h>
#include <logger/logger.h>
#include <rng/rng.h>
#include <scripter/scripter.h>
#include <Player.h>
#include <Stat.h>

namespace {
namespace fs = std::filesystem;

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

std::string toLower(std::string value) {
    for (char& ch : value) {
        ch = static_cast<char>(std::tolower(static_cast<unsigned char>(ch)));
    }
    return value;
}

const char* attrToString(AttrType attr) {
    if (attr == AttrType::None) {
        return "None";
    }
    const auto idx = static_cast<std::size_t>(attr);
    if (idx < static_cast<std::size_t>(AttrType::COUNT)) {
        return Attr2StringEN[idx].c_str();
    }
    return "None";
}

bool isAssetsDir(const fs::path& dir) {
    return fs::exists(dir / "pets") && fs::exists(dir / "skills");
}

std::optional<fs::path> findAssetsRoot() {
    fs::path current = fs::current_path();
    for (fs::path dir = current; !dir.empty(); dir = dir.parent_path()) {
        fs::path direct = dir / "assets";
        if (isAssetsDir(direct)) return direct;
        fs::path nested = dir / "arena" / "assets";
        if (isAssetsDir(nested)) return nested;
        if (dir == dir.parent_path()) break;
    }
    return std::nullopt;
}

fs::path resolveAssetPath(const fs::path& requested, const std::optional<fs::path>& assetsRoot) {
    if (requested.empty() || requested.is_absolute() || fs::exists(requested) || !assetsRoot) {
        return requested;
    }

    const std::string reqStr = requested.generic_string();
    const std::string marker = "assets/";
    auto pos = reqStr.find(marker);
    if (pos != std::string::npos) {
        const std::string suffix = reqStr.substr(pos + marker.size());
        return *assetsRoot / suffix;
    }
    return *assetsRoot / requested;
}

std::optional<Ailment> parseAilment(const std::string& name) {
    const std::string lowered = toLower(name);
    if (lowered == "trapped") return Ailment::Trapped;
    if (lowered == "deepsleep") return Ailment::DeepSleep;
    if (lowered == "bewitch") return Ailment::Bewitch;
    if (lowered == "curse") return Ailment::Curse;
    if (lowered == "parasite") return Ailment::Parasite;
    if (lowered == "fear") return Ailment::Fear;
    if (lowered == "confusion") return Ailment::Confusion;
    if (lowered == "toxic") return Ailment::Toxic;
    if (lowered == "poison") return Ailment::Poison;
    if (lowered == "freeze") return Ailment::Freeze;
    if (lowered == "burn") return Ailment::Burn;
    if (lowered == "paralysis") return Ailment::Paralysis;
    if (lowered == "sleep") return Ailment::Sleep;
    if (lowered == "none") return Ailment::None;
    return std::nullopt;
}

const char* ailmentToString(Ailment status) {
    switch (status) {
        case Ailment::Trapped:
            return "Trapped";
        case Ailment::DeepSleep:
            return "DeepSleep";
        case Ailment::Bewitch:
            return "Bewitch";
        case Ailment::Curse:
            return "Curse";
        case Ailment::Parasite:
            return "Parasite";
        case Ailment::Fear:
            return "Fear";
        case Ailment::Confusion:
            return "Confusion";
        case Ailment::Toxic:
            return "Toxic";
        case Ailment::Poison:
            return "Poison";
        case Ailment::Freeze:
            return "Freeze";
        case Ailment::Burn:
            return "Burn";
        case Ailment::Paralysis:
            return "Paralysis";
        case Ailment::Sleep:
            return "Sleep";
        case Ailment::None:
        case Ailment::Count:
        default:
            return "None";
    }
}

std::optional<Stat> parseStat(const std::string& name) {
    const std::string lowered = toLower(name);
    if (lowered == "atk" || lowered == "attack") return Stat::Atk;
    if (lowered == "def" || lowered == "defense") return Stat::Def;
    if (lowered == "spa" || lowered == "spatk" || lowered == "sp_attack") return Stat::SpA;
    if (lowered == "spd" || lowered == "spdef" || lowered == "sp_defense") return Stat::SpD;
    if (lowered == "spe" || lowered == "speed") return Stat::Spe;
    if (lowered == "acc" || lowered == "accuracy") return Stat::Acc;
    if (lowered == "eva" || lowered == "evasion") return Stat::Eva;
    if (lowered == "cri" || lowered == "crit" || lowered == "critical") return Stat::Cri;
    return std::nullopt;
}

std::optional<AttrType> parseAttr(const std::string& name) {
    if (name == "None" || name == "none") return AttrType::None;
    if (auto it = AttrFromEN.find(name); it != AttrFromEN.end()) return it->second;
    if (auto it = AttrFromZH.find(name); it != AttrFromZH.end()) return it->second;
    return std::nullopt;
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
        scripter.registerFunction("deal_power_damage", [&]() {
            targetPet.takeDamage(calculatePowerDamage(*skill_, selfPet, targetPet));
        });
        scripter.registerFunction("deal_power_damage_scaled", [&](double scale) {
            const int base = calculatePowerDamage(*skill_, selfPet, targetPet);
            targetPet.takeDamage(static_cast<int>(base * scale));
        });
        scripter.registerFunction("heal_self", [&](int amount) { selfPet.restoreHP(amount); });
        scripter.registerFunction("heal_target", [&](int amount) { targetPet.restoreHP(amount); });
        scripter.registerFunction("minus_pp", [&](int amount) { return selfPet.consumePP(skill_->id(), amount); });
        scripter.registerFunction("get_self_last_damage_taken", [&]() { return selfPet.lastDamageTaken(); });
        scripter.registerFunction("get_target_last_damage_taken", [&]() { return targetPet.lastDamageTaken(); });
        scripter.registerFunction("get_self_turn_damage_taken", [&]() { return selfPet.turnDamageTaken(); });
        scripter.registerFunction("get_target_turn_damage_taken", [&]() { return targetPet.turnDamageTaken(); });
        scripter.registerFunction("set_self_damage_multiplier", [&](double multiplier) {
            selfPet.setDamageMultiplier(multiplier);
        });
        scripter.registerFunction("set_target_damage_multiplier", [&](double multiplier) {
            targetPet.setDamageMultiplier(multiplier);
        });
        scripter.registerFunction("set_self_damage_multiplier_turns", [&](double multiplier, int turns) {
            selfPet.setDamageMultiplierTurns(multiplier, turns);
        });
        scripter.registerFunction("set_target_damage_multiplier_turns", [&](double multiplier, int turns) {
            targetPet.setDamageMultiplierTurns(multiplier, turns);
        });
        scripter.registerFunction("set_self_damage_immunity_one_turn", [&]() {
            selfPet.setDamageImmunityTurns(1);
        });
        scripter.registerFunction("set_target_damage_immunity_one_turn", [&]() {
            targetPet.setDamageImmunityTurns(1);
        });
        scripter.registerFunction("set_self_flat_damage_reduction", [&](int amount) {
            selfPet.setFlatDamageReduction(amount);
        });
        scripter.registerFunction("set_target_flat_damage_reduction", [&](int amount) {
            targetPet.setFlatDamageReduction(amount);
        });
        scripter.registerFunction("set_self_per_hit_damage_reduction", [&](int amount, int turns) {
            selfPet.setPerHitDamageReduction(amount, turns);
        });
        scripter.registerFunction("set_target_per_hit_damage_reduction", [&](int amount, int turns) {
            targetPet.setPerHitDamageReduction(amount, turns);
        });
        scripter.registerFunction("rand_int", [&](int minVal, int maxVal) {
            return RNG::instance().range<int>(minVal, maxVal);
        });
        scripter.registerFunction("apply_self_ailment", [&](const std::string& name) {
            auto ailment = parseAilment(name);
            if (!ailment.has_value()) return false;
            return selfPet.buff().applyAilmentWithEffects(*ailment, selfPet.attrs(), selfPet, &targetPet);
        });
        scripter.registerFunction("apply_target_ailment", [&](const std::string& name) {
            auto ailment = parseAilment(name);
            if (!ailment.has_value()) return false;
            return targetPet.buff().applyAilmentWithEffects(*ailment, targetPet.attrs(), targetPet, &selfPet);
        });
        scripter.registerFunction("clear_self_ailments", [&]() { selfPet.buff().clearAilments(); });
        scripter.registerFunction("clear_target_ailments", [&]() { targetPet.buff().clearAilments(); });
        scripter.registerFunction("has_self_ailment", [&](const std::string& name) {
            auto ailment = parseAilment(name);
            if (!ailment.has_value()) return false;
            return selfPet.buff().hasAilment(*ailment);
        });
        scripter.registerFunction("has_target_ailment", [&](const std::string& name) {
            auto ailment = parseAilment(name);
            if (!ailment.has_value()) return false;
            return targetPet.buff().hasAilment(*ailment);
        });
        scripter.registerFunction("get_self_primary_ailment", [&]() {
            return std::string(ailmentToString(selfPet.buff().primaryAilment()));
        });
        scripter.registerFunction("get_self_secondary_ailment", [&]() {
            return std::string(ailmentToString(selfPet.buff().secondaryAilment()));
        });
        scripter.registerFunction("get_target_primary_ailment", [&]() {
            return std::string(ailmentToString(targetPet.buff().primaryAilment()));
        });
        scripter.registerFunction("get_target_secondary_ailment", [&]() {
            return std::string(ailmentToString(targetPet.buff().secondaryAilment()));
        });
        scripter.registerFunction("get_self_stage", [&](const std::string& statName) {
            auto stat = parseStat(statName);
            if (!stat.has_value()) return 0;
            return selfPet.buff().stage(*stat);
        });
        scripter.registerFunction("get_target_stage", [&](const std::string& statName) {
            auto stat = parseStat(statName);
            if (!stat.has_value()) return 0;
            return targetPet.buff().stage(*stat);
        });
        scripter.registerFunction("change_self_stage", [&](const std::string& statName, int delta) {
            auto stat = parseStat(statName);
            if (!stat.has_value()) return 0;
            return selfPet.buff().changeStage(*stat, delta, nullptr);
        });
        scripter.registerFunction("change_target_stage", [&](const std::string& statName, int delta) {
            auto stat = parseStat(statName);
            if (!stat.has_value()) return 0;
            return targetPet.buff().changeStage(*stat, delta, nullptr);
        });
        scripter.registerFunction("raise_self_stage", [&](const std::string& statName, int amount) {
            auto stat = parseStat(statName);
            if (!stat.has_value()) return 0;
            const int delta = std::max(0, amount);
            return selfPet.buff().changeStage(*stat, delta, nullptr);
        });
        scripter.registerFunction("lower_self_stage", [&](const std::string& statName, int amount) {
            auto stat = parseStat(statName);
            if (!stat.has_value()) return 0;
            const int delta = -std::max(0, amount);
            return selfPet.buff().changeStage(*stat, delta, nullptr);
        });
        scripter.registerFunction("raise_target_stage", [&](const std::string& statName, int amount) {
            auto stat = parseStat(statName);
            if (!stat.has_value()) return 0;
            const int delta = std::max(0, amount);
            return targetPet.buff().changeStage(*stat, delta, nullptr);
        });
        scripter.registerFunction("lower_target_stage", [&](const std::string& statName, int amount) {
            auto stat = parseStat(statName);
            if (!stat.has_value()) return 0;
            const int delta = -std::max(0, amount);
            return targetPet.buff().changeStage(*stat, delta, nullptr);
        });
        scripter.registerFunction("get_attr_multiplier",
                                  [&](const std::string& atkName, const std::string& def1, const std::string& def2) {
                                      auto atkAttr = parseAttr(atkName);
                                      auto defAttr1 = parseAttr(def1);
                                      auto defAttr2 = parseAttr(def2);
                                      if (!atkAttr.has_value()) return 1.0;
                                      AttrType d1 = defAttr1.value_or(AttrType::None);
                                      AttrType d2 = defAttr2.value_or(AttrType::None);
                                      return AttrChart::getAttrAdvantage(*atkAttr, std::array<AttrType, 2>{ d1, d2 });
                                  });

        scripter.set("skill_id", skill_->id());
        scripter.set("skill_name", skill_->name());
        scripter.set("skill_type", SkillTypeTable[static_cast<int>(skill_->skillType())]);
        scripter.set("skill_attr", attrToString(skill_->skillAttr()));
        scripter.set("skill_power", skill_->skillPower());
        scripter.set("skill_priority", skill_->skillPriority());
        scripter.set("attacker_hp", selfPet.currentHP());
        scripter.set("attacker_max_hp", selfPet.maxHP());
        scripter.set("attacker_level", selfPet.level());
        scripter.set("attacker_attr1", attrToString(selfPet.attrs()[0]));
        scripter.set("attacker_attr2", attrToString(selfPet.attrs()[1]));
        scripter.set("attacker_attack", selfPet.stagedAttack());
        scripter.set("attacker_defense", selfPet.stagedDefense());
        scripter.set("attacker_sp_attack", selfPet.stagedSpecialAttack());
        scripter.set("attacker_sp_defense", selfPet.stagedSpecialDefense());
        scripter.set("attacker_speed", selfPet.currentSpeed());
        scripter.set("attacker_attack_base", selfPet.attack());
        scripter.set("attacker_defense_base", selfPet.defense());
        scripter.set("attacker_sp_attack_base", selfPet.specialAttack());
        scripter.set("attacker_sp_defense_base", selfPet.specialDefense());
        scripter.set("attacker_speed_base", selfPet.getRS().rSpe);
        scripter.set("target_hp", targetPet.currentHP());
        scripter.set("target_max_hp", targetPet.maxHP());
        scripter.set("target_level", targetPet.level());
        scripter.set("target_attr1", attrToString(targetPet.attrs()[0]));
        scripter.set("target_attr2", attrToString(targetPet.attrs()[1]));
        scripter.set("target_attack", targetPet.stagedAttack());
        scripter.set("target_defense", targetPet.stagedDefense());
        scripter.set("target_sp_attack", targetPet.stagedSpecialAttack());
        scripter.set("target_sp_defense", targetPet.stagedSpecialDefense());
        scripter.set("target_speed", targetPet.currentSpeed());
        scripter.set("target_attack_base", targetPet.attack());
        scripter.set("target_defense_base", targetPet.defense());
        scripter.set("target_sp_attack_base", targetPet.specialAttack());
        scripter.set("target_sp_defense_base", targetPet.specialDefense());
        scripter.set("target_speed_base", targetPet.getRS().rSpe);

        auto assetsRoot = findAssetsRoot();
        fs::path resolvedScript = resolveAssetPath(scriptPath, assetsRoot);
        const std::string resolvedScriptPath = resolvedScript.empty() ? scriptPath : resolvedScript.string();

        if (!scripter.runScript(resolvedScriptPath)) {
            LOG_ERROR(module(), "Failed to run script: ", resolvedScriptPath);
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
