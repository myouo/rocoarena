#pragma once

#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <random>
#include <string>
#include <vector>

#include <battle/BattleSystem.h>
#include <battle/Action.h>
#include <battle/SkillAction.h>
#include <entity/Player.h>
#include <entity/Pet.h>
#include <nlohmann/json.hpp>
#include <skill/SkillRegistry.h>

struct ActionData {
    ActionType type = ActionType::Stay;
    int skillId = 0;
    std::size_t switchIndex = 0;
};

enum class PendingType {
    ChooseAction,
    ForceSwitch,
    Wait,
    BattleOver
};

struct BattleOutcome {
    bool ended = false;
    int winner = 0; // 0 = none, 1 = player1, 2 = player2
    std::string reason;
};

struct ResolvedActions {
    int turn = 0;
    ActionData p1;
    ActionData p2;
};

class BattleSession {
  public:
    BattleSession(std::vector<std::unique_ptr<Pet>> roster1, std::vector<std::unique_ptr<Pet>> roster2,
                  const SkillRegistry& registry);

    PendingType pendingForPlayer(int index) const;
    bool submitAction(int index, const ActionData& action, std::string* error = nullptr);
    void tick();
    void forfeit(int index);

    BattleOutcome outcome() const { return outcome_; }
    int currentTurn() const { return battle_.currentTurn(); }

    nlohmann::json stateForPlayer(int index) const;
    nlohmann::json spectatorState() const;

    Player& player1() { return player1_; }
    Player& player2() { return player2_; }
    const std::vector<std::unique_ptr<Pet>>& roster1() const { return roster1_; }
    const std::vector<std::unique_ptr<Pet>>& roster2() const { return roster2_; }

  private:
    using Clock = std::chrono::steady_clock;

    bool needsForceSwitch(int index) const;
    void scheduleForceSwitch(int index);
    void scheduleTurnDeadline();
    void clearTurnDeadline();
    void applyRandomSwitch(int index);
    ActionData buildRandomSkillAction(int index) const;
    void applyRandomSkill(int index);
    void resolveTurn();
    bool validateAction(int index, const ActionData& action, std::string* error) const;
    std::unique_ptr<Action> buildAction(const ActionData& action) const;

    void updateOutcome();
    nlohmann::json actionToJson(const ActionData& action) const;

    std::vector<std::unique_ptr<Pet>> roster1_;
    std::vector<std::unique_ptr<Pet>> roster2_;
    Player player1_;
    Player player2_;
    BattleSystem battle_{};
    const SkillRegistry* registry_ = nullptr;

    std::array<std::optional<ActionData>, 2> actions_{};
    std::array<bool, 2> forcePending_{};
    std::array<Clock::time_point, 2> forceDeadline_{};
    std::optional<Clock::time_point> turnDeadline_{};

    std::optional<int> lastFlee_;
    std::optional<ResolvedActions> lastResolved_;
    BattleOutcome outcome_{};

    mutable std::mt19937 rng_{ std::random_device{}() };
};
