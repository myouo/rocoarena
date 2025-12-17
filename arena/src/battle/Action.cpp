#include "Action.h"

#include <logger/logger.h>

#include <Player.h>

#include "BattleSystem.h"

SwitchAction::SwitchAction(std::size_t targetIndex) : Action(ActionType::Switch), targetIndex_(targetIndex) {}

void SwitchAction::execute(BattleSystem& /*battle*/, Player& self, Player& /*opponent*/) {
    if (!self.switchTo(targetIndex_)) {
        LOG_WARN(module(), "Switch action failed for target index ", targetIndex_);
    }
}

void PotionAction::execute(BattleSystem& /*battle*/, Player& self, Player& /*opponent*/) {
    if (healAmount_ <= 0) {
        LOG_WARN(module(), "Potion heal amount is non-positive, skipping.");
        return;
    }
    Pet& pet = self.activePet();
    pet.restoreHP(healAmount_);
    LOG_INFO(module(), "Healed for ", healAmount_, " HP. Current HP=", pet.currentHP(), "/", pet.maxHP());
}

void FleeAction::execute(BattleSystem& battle, Player& /*self*/, Player& /*opponent*/) {
    battle.endBattle("fled");
    LOG_INFO(module(), "Fled from battle.");
}

void StayAction::execute(BattleSystem& /*battle*/, Player& /*self*/, Player& /*opponent*/) {
    LOG_INFO(module(), "Stay: skipping action.");
}
