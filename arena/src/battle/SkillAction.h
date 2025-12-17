#pragma once

#include <logger/logger.h>
#include <skill/SkillBase.h>

#include "Action.h"

// Action wrapper for casting a specific skill.
class SkillAction : public Action {
  public:
    SkillAction(const SkillBase& skill) : Action(ActionType::Skill), skill_(&skill) {}

    const SkillBase& skill() const { return *skill_; }
    int priority() const override { return skill_->skillPriority(); }
    bool guaranteedHit() const { return skill_->isGuaranteedHit(); }
    int id() const { return skill_->id(); }

    void execute(BattleSystem& battle, Player& self, Player& opponent) override;

  private:
    static constexpr const char* module() { return "SkillAction"; }
    const SkillBase* skill_;
};
