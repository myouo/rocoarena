#pragma once

#include <string>
#include <utility>

#include <Pet.h>
#include <forward.h>

class BattleSystem {
  public:
    void init(Player& p1, Player& p2);
    void takeTurn(Action& action1, Action& action2);

    bool isBattleOver() const { return battleEnded_; }
    void endBattle(const std::string& reason = {});
    const std::string& endReason() const { return endReason_; }

    int currentTurn() const { return turnCounter_; }

  private:
    void onTurnStart(Player& p1, Player& p2);
    void onTurnEnd(Player& p1, Player& p2);

    std::pair<Action*, Action*> decideOrder(Action& action1, Action& action2, Pet& pet1, Pet& pet2);
    static constexpr const char* module() { return "BattleSystem"; }

    Player* player1_ = nullptr;
    Player* player2_ = nullptr;
    bool battleEnded_ = false;
    std::string endReason_;

    int turnCounter_ = 0;
};
