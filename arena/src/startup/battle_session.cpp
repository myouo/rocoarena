#include "battle_session.h"

#include <algorithm>
#include <sstream>

#include <logger/logger.h>

namespace {
constexpr std::chrono::seconds kForceSwitchTimeout{10};
constexpr std::chrono::seconds kChooseActionTimeout{10};

std::vector<std::size_t> usableIndices(const std::vector<std::unique_ptr<Pet>>& roster) {
    std::vector<std::size_t> out;
    for (std::size_t i = 0; i < roster.size(); ++i) {
        if (roster[i] && !roster[i]->isFainted()) {
            out.push_back(i);
        }
    }
    return out;
}
} // namespace

BattleSession::BattleSession(std::vector<std::unique_ptr<Pet>> roster1, std::vector<std::unique_ptr<Pet>> roster2,
                             const SkillRegistry& registry)
    : roster1_(std::move(roster1)), roster2_(std::move(roster2)),
      player1_([&]() {
          Player::Roster roster{};
          roster.fill(nullptr);
          for (std::size_t i = 0; i < roster1_.size() && i < Player::kMaxPets; ++i) {
              roster[i] = roster1_[i].get();
          }
          return roster;
      }()),
      player2_([&]() {
          Player::Roster roster{};
          roster.fill(nullptr);
          for (std::size_t i = 0; i < roster2_.size() && i < Player::kMaxPets; ++i) {
              roster[i] = roster2_[i].get();
          }
          return roster;
      }()),
      registry_(&registry) {
    battle_.init(player1_, player2_);
    forcePending_.fill(false);
}

PendingType BattleSession::pendingForPlayer(int index) const {
    if (outcome_.ended) return PendingType::BattleOver;
    if (index < 0 || index > 1) return PendingType::BattleOver;
    if (forcePending_[index] || needsForceSwitch(index)) return PendingType::ForceSwitch;
    if (actions_[index].has_value()) return PendingType::Wait;
    return PendingType::ChooseAction;
}

bool BattleSession::needsForceSwitch(int index) const {
    const Player& player = (index == 0) ? player1_ : player2_;
    if (!player.hasUsablePets()) return false;
    try {
        return player.activePet().isFainted();
    } catch (...) {
        return false;
    }
}

void BattleSession::scheduleForceSwitch(int index) {
    forcePending_[index] = true;
    forceDeadline_[index] = Clock::now() + kForceSwitchTimeout;
    actions_[index].reset();
    clearTurnDeadline();
}

void BattleSession::scheduleTurnDeadline() {
    if (turnDeadline_.has_value()) return;
    turnDeadline_ = Clock::now() + kChooseActionTimeout;
}

void BattleSession::clearTurnDeadline() {
    turnDeadline_.reset();
}

bool BattleSession::validateAction(int index, const ActionData& action, std::string* error) const {
    const Player& player = (index == 0) ? player1_ : player2_;

    if (action.type == ActionType::Potion) {
        if (error) *error = "potion action is disabled";
        return false;
    }

    if (action.type == ActionType::Switch) {
        if (action.switchIndex >= Player::kMaxPets) {
            if (error) *error = "switch index out of range";
            return false;
        }
        const auto& roster = (index == 0) ? roster1_ : roster2_;
        if (action.switchIndex >= roster.size() || !roster[action.switchIndex]) {
            if (error) *error = "switch target missing";
            return false;
        }
        if (roster[action.switchIndex]->isFainted()) {
            if (error) *error = "switch target fainted";
            return false;
        }
        return true;
    }

    if (action.type == ActionType::Skill) {
        const Pet& pet = player.activePet();
        bool canUse = false;
        for (const auto& slot : pet.learnedSkills()) {
            if (slot.has_value() && slot->base && slot->base->id() == action.skillId) {
                canUse = true;
                break;
            }
        }
        if (!canUse) {
            if (error) *error = "skill not learned";
            return false;
        }
        return true;
    }

    if (action.type == ActionType::Flee || action.type == ActionType::Stay) {
        return true;
    }

    if (error) *error = "invalid action";
    return false;
}

bool BattleSession::submitAction(int index, const ActionData& action, std::string* error) {
    if (outcome_.ended) {
        if (error) *error = "battle already ended";
        return false;
    }
    if (index < 0 || index > 1) {
        if (error) *error = "invalid player index";
        return false;
    }

    if (forcePending_[index] || needsForceSwitch(index)) {
        if (action.type != ActionType::Switch) {
            if (error) *error = "must switch due to fainted pet";
            return false;
        }
        if (!validateAction(index, action, error)) return false;
        Player& player = (index == 0) ? player1_ : player2_;
        if (!player.switchTo(action.switchIndex)) {
            if (error) *error = "switch failed";
            return false;
        }
        forcePending_[index] = false;
        actions_[index].reset();
        return true;
    }

    if (!validateAction(index, action, error)) return false;
    actions_[index] = action;
    return true;
}

std::unique_ptr<Action> BattleSession::buildAction(const ActionData& action) const {
    switch (action.type) {
        case ActionType::Skill: {
            const SkillBase* skill = registry_->get(action.skillId);
            if (!skill) {
                return std::make_unique<StayAction>();
            }
            return std::make_unique<SkillAction>(*skill);
        }
        case ActionType::Switch:
            return std::make_unique<SwitchAction>(action.switchIndex);
        case ActionType::Flee:
            return std::make_unique<FleeAction>();
        case ActionType::Stay:
        default:
            return std::make_unique<StayAction>();
    }
}

void BattleSession::applyRandomSwitch(int index) {
    auto indices = usableIndices(index == 0 ? roster1_ : roster2_);
    if (indices.empty()) return;
    std::uniform_int_distribution<std::size_t> dist(0, indices.size() - 1);
    std::size_t pick = indices[dist(rng_)];

    Player& player = (index == 0) ? player1_ : player2_;
    if (player.switchTo(pick)) {
        forcePending_[index] = false;
        actions_[index].reset();
    }
}

ActionData BattleSession::buildRandomSkillAction(int index) const {
    const Player& player = (index == 0) ? player1_ : player2_;
    std::vector<int> skills;
    try {
        for (const auto& slot : player.activePet().learnedSkills()) {
            if (slot.has_value() && slot->base) {
                skills.push_back(slot->base->id());
            }
        }
    } catch (...) {
        return ActionData{ ActionType::Stay, 0, 0 };
    }

    if (skills.empty()) {
        return ActionData{ ActionType::Stay, 0, 0 };
    }
    std::uniform_int_distribution<std::size_t> dist(0, skills.size() - 1);
    return ActionData{ ActionType::Skill, skills[dist(rng_)], 0 };
}

void BattleSession::applyRandomSkill(int index) {
    if (actions_[index].has_value()) return;
    ActionData action = buildRandomSkillAction(index);
    actions_[index] = action;
    if (action.type == ActionType::Skill) {
        LOG_INFO("BattleSession", "Action timeout: auto skill for player ", index + 1, " skillId=", action.skillId);
    } else {
        LOG_INFO("BattleSession", "Action timeout: auto stay for player ", index + 1);
    }
}

void BattleSession::resolveTurn() {
    if (!actions_[0].has_value() || !actions_[1].has_value()) return;

    ActionData a1 = *actions_[0];
    ActionData a2 = *actions_[1];

    if (a1.type == ActionType::Flee) lastFlee_ = 0;
    if (a2.type == ActionType::Flee) lastFlee_ = 1;

    auto act1 = buildAction(a1);
    auto act2 = buildAction(a2);

    battle_.takeTurn(*act1, *act2);
    lastResolved_ = ResolvedActions{ battle_.currentTurn(), a1, a2 };

    actions_[0].reset();
    actions_[1].reset();
    clearTurnDeadline();

    updateOutcome();

    if (needsForceSwitch(0)) scheduleForceSwitch(0);
    if (needsForceSwitch(1)) scheduleForceSwitch(1);
}

void BattleSession::updateOutcome() {
    if (outcome_.ended) return;

    if (battle_.isBattleOver()) {
        outcome_.ended = true;
        outcome_.reason = battle_.endReason();
        if (lastFlee_.has_value()) {
            outcome_.winner = (*lastFlee_ == 0) ? 2 : 1;
            return;
        }
        if (!player1_.hasUsablePets() && player2_.hasUsablePets()) {
            outcome_.winner = 2;
            return;
        }
        if (!player2_.hasUsablePets() && player1_.hasUsablePets()) {
            outcome_.winner = 1;
            return;
        }
        outcome_.winner = 0;
    }
}

void BattleSession::forfeit(int index) {
    if (outcome_.ended) return;
    outcome_.ended = true;
    outcome_.reason = "player left";
    if (index == 0) {
        outcome_.winner = 2;
    } else if (index == 1) {
        outcome_.winner = 1;
    } else {
        outcome_.winner = 0;
    }
}

void BattleSession::tick() {
    if (outcome_.ended) return;

    if (!player1_.hasUsablePets() || !player2_.hasUsablePets()) {
        if (!battle_.isBattleOver()) {
            battle_.endBattle("all pets fainted");
        }
        updateOutcome();
        return;
    }

    for (int i = 0; i < 2; ++i) {
        if (!forcePending_[i] && needsForceSwitch(i)) {
            scheduleForceSwitch(i);
        }
        if (forcePending_[i]) {
            if (Clock::now() >= forceDeadline_[i]) {
                applyRandomSwitch(i);
            }
            continue;
        }
    }

    if (!forcePending_[0] && !forcePending_[1]) {
        if (!actions_[0].has_value() || !actions_[1].has_value()) {
            scheduleTurnDeadline();
        }
        if (turnDeadline_.has_value() && Clock::now() >= *turnDeadline_) {
            if (!actions_[0].has_value()) {
                applyRandomSkill(0);
            }
            if (!actions_[1].has_value()) {
                applyRandomSkill(1);
            }
        }
        if (actions_[0].has_value() && actions_[1].has_value()) {
            resolveTurn();
        }
    }

    updateOutcome();
}

nlohmann::json BattleSession::actionToJson(const ActionData& action) const {
    nlohmann::json j;
    switch (action.type) {
        case ActionType::Skill:
            j["type"] = "skill";
            j["skillId"] = action.skillId;
            break;
        case ActionType::Switch:
            j["type"] = "switch";
            j["index"] = action.switchIndex;
            break;
        case ActionType::Flee:
            j["type"] = "flee";
            break;
        case ActionType::Stay:
        default:
            j["type"] = "stay";
            break;
    }
    return j;
}

nlohmann::json BattleSession::stateForPlayer(int index) const {
    nlohmann::json state;
    state["turn"] = battle_.currentTurn();
    state["battleOver"] = outcome_.ended;
    state["winner"] = outcome_.winner;
    state["reason"] = outcome_.reason;

    PendingType pending = pendingForPlayer(index);
    switch (pending) {
        case PendingType::ChooseAction:
            state["pending"] = "choose_action";
            break;
        case PendingType::ForceSwitch:
            state["pending"] = "force_switch";
            break;
        case PendingType::Wait:
            state["pending"] = "wait";
            break;
        case PendingType::BattleOver:
        default:
            state["pending"] = "battle_over";
            break;
    }

    auto buildPetJson = [&](const Pet& pet, bool includeSkills) {
        nlohmann::json j;
        j["hp"] = pet.currentHP();
        j["maxHp"] = pet.maxHP();
        j["fainted"] = pet.isFainted();
        j["attrs"] = { static_cast<int>(pet.attrs()[0]), static_cast<int>(pet.attrs()[1]) };
        if (includeSkills) {
            j["skills"] = nlohmann::json::array();
            for (const auto& slot : pet.learnedSkills()) {
                if (!slot.has_value() || !slot->base) continue;
                nlohmann::json sk;
                sk["id"] = slot->base->id();
                sk["name"] = slot->base->name();
                sk["pp"] = slot->currentPP;
                sk["maxPP"] = slot->base->skillMaxPP();
                j["skills"].push_back(std::move(sk));
            }
        }
        return j;
    };

    const Player& self = (index == 0) ? player1_ : player2_;
    const Player& opponent = (index == 0) ? player2_ : player1_;
    const auto& roster = (index == 0) ? roster1_ : roster2_;

    nlohmann::json selfJson;
    selfJson["activeIndex"] = self.activeIndex();
    selfJson["pets"] = nlohmann::json::array();

    for (std::size_t i = 0; i < roster.size(); ++i) {
        if (!roster[i]) continue;
        nlohmann::json petJson = buildPetJson(*roster[i], true);
        petJson["index"] = i;
        petJson["name"] = roster[i]->name();
        selfJson["pets"].push_back(std::move(petJson));
    }

    nlohmann::json oppJson;
    oppJson["activeIndex"] = opponent.activeIndex();
    try {
        nlohmann::json activeJson = buildPetJson(opponent.activePet(), true);
        activeJson["name"] = opponent.activePet().name();
        oppJson["active"] = std::move(activeJson);
    } catch (...) {
        oppJson["active"] = nlohmann::json::object();
    }

    state["self"] = std::move(selfJson);
    state["opponent"] = std::move(oppJson);

    return state;
}

nlohmann::json BattleSession::spectatorState() const {
    nlohmann::json state = stateForPlayer(0);
    if (lastResolved_.has_value()) {
        nlohmann::json actions;
        actions["turn"] = lastResolved_->turn;
        actions["p1"] = actionToJson(lastResolved_->p1);
        actions["p2"] = actionToJson(lastResolved_->p2);
        state["lastActions"] = std::move(actions);
    } else {
        state["lastActions"] = nlohmann::json::object();
    }
    state["spectator"] = true;
    return state;
}
