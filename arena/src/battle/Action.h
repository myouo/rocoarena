#pragma once

#include <cstddef>

#include <forward.h>

enum class ActionType { Skill, Switch, Potion, Flee, Stay };

class Action {
  public:
    Action(ActionType type) : type_(type){};
    virtual ~Action() = default;

    ActionType getType() const { return type_; }
    // Base priority hook; override in subclasses.
    virtual int priority() const { return 0; }

    virtual void execute(BattleSystem& battle, Player& self, Player& opponent) = 0;

  private:
    ActionType type_;
};

// —— Switch —— //
class SwitchAction : public Action {
  public:
    explicit SwitchAction(std::size_t targetIndex);
    int priority() const override { return 8; }
    void execute(BattleSystem& battle, Player& self, Player& opponent) override;
    std::size_t targetIndex() const { return targetIndex_; }

  private:
    static constexpr const char* module() { return "SwitchAction"; }
    std::size_t targetIndex_;
};

// —— Potion —— //
class PotionAction : public Action {
  public:
    explicit PotionAction(int healAmount) : Action(ActionType::Potion), healAmount_(healAmount) {}
    int priority() const override { return 8; }
    void execute(BattleSystem& battle, Player& self, Player& opponent) override;

  private:
    static constexpr const char* module() { return "PotionAction"; }
    int healAmount_;
};

// —— Flee —— //
class FleeAction : public Action {
  public:
    FleeAction() : Action(ActionType::Flee) {}
    int priority() const override { return 16; }
    void execute(BattleSystem& battle, Player& self, Player& opponent) override;

  private:
    static constexpr const char* module() { return "FleeAction"; }
};

// —— Stay —— //
class StayAction : public Action {
  public:
    StayAction() : Action(ActionType::Stay) {}
    int priority() const override { return 8; }
    void execute(BattleSystem& battle, Player& self, Player& opponent) override;

  private:
    static constexpr const char* module() { return "StayAction"; }
};
