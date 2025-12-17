#include "BattleSystem.h"

#include <utility>

#include <logger/logger.h>
#include <rng/rng.h>

#include <Player.h>

#include "Action.h"

void BattleSystem::init(Player& p1, Player& p2) {
    player1_ = &p1;
    player2_ = &p2;
    battleEnded_ = false;
    endReason_.clear();
    turnCounter_ = 0;
}

void BattleSystem::endBattle(const std::string& reason) {
    battleEnded_ = true;
    endReason_ = reason;
    LOG_INFO(module(), "Battle ended. Reason: ", reason);
}

std::pair<Action*, Action*> BattleSystem::decideOrder(Action& action1, Action& action2, Pet& pet1, Pet& pet2) {
    const int p1Priority = action1.priority();
    const int p2Priority = action2.priority();

    if (p1Priority != p2Priority) {
        return (p1Priority > p2Priority) ? std::pair<Action*, Action*>{ &action1, &action2 }
                                         : std::pair<Action*, Action*>{ &action2, &action1 };
    }

    const int p1Speed = pet1.currentSpeed();
    const int p2Speed = pet2.currentSpeed();
    if (p1Speed != p2Speed) {
        return (p1Speed > p2Speed) ? std::pair<Action*, Action*>{ &action1, &action2 }
                                   : std::pair<Action*, Action*>{ &action2, &action1 };
    }

    const bool firstIsP1 = RNG::instance().range<int>(0, 1) == 0;
    return firstIsP1 ? std::pair<Action*, Action*>{ &action1, &action2 }
                     : std::pair<Action*, Action*>{ &action2, &action1 };
}

void BattleSystem::takeTurn(Action& action1, Action& action2) {
    if (battleEnded_ || !player1_ || !player2_) {
        LOG_WARN(module(), "Battle not initialized or already ended.");
        return;
    }

    if (!player1_->hasUsablePets()) {
        endBattle("Player1 has no usable pets.");
        return;
    }
    if (!player2_->hasUsablePets()) {
        endBattle("Player2 has no usable pets.");
        return;
    }

    // Ensure both sides have an active, usable pet before the turn starts.
    if (!player1_->ensureActiveUsable()) {
        endBattle("Player1 cannot field a usable pet.");
        return;
    }
    if (!player2_->ensureActiveUsable()) {
        endBattle("Player2 cannot field a usable pet.");
        return;
    }

    ++turnCounter_;
    LOG_INFO(module(), "Starting turn ", turnCounter_);

    Pet& pet1 = player1_->activePet();
    Pet& pet2 = player2_->activePet();
    auto order = decideOrder(action1, action2, pet1, pet2);

    Action* first = order.first;
    Action* second = order.second;

    Player* firstPlayer = (first == &action1) ? player1_ : player2_;
    Player* secondPlayer = (first == &action1) ? player2_ : player1_;

    first->execute(*this, *firstPlayer, *secondPlayer);
    if (battleEnded_) return;

    if (!secondPlayer->hasUsablePets()) {
        endBattle("All opponent pets fainted.");
        return;
    }

    if (secondPlayer->activePet().isFainted()) {
        LOG_INFO(module(), "Second actor's active pet fainted; skipping action.");
        return;
    }

    second->execute(*this, *secondPlayer, *firstPlayer);
    if (battleEnded_) return;

    if (!firstPlayer->hasUsablePets()) {
        endBattle("All opponent pets fainted.");
    } else if (!secondPlayer->hasUsablePets()) {
        endBattle("All opponent pets fainted.");
    }
}
