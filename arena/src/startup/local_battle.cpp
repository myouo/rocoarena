#include "local_battle.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <random>
#include <sstream>

#include "battle_session.h"
#include "cli_helpers.h"
#include "data_loader.h"

namespace {
std::vector<std::unique_ptr<Pet>> buildRandomRoster(const DataStore& store, std::mt19937& rng, std::size_t count) {
    std::vector<int> ids;
    ids.reserve(store.pets.size());
    for (const auto& it : store.pets) {
        ids.push_back(it.first);
    }
    std::shuffle(ids.begin(), ids.end(), rng);

    std::vector<std::unique_ptr<Pet>> roster;
    for (std::size_t i = 0; i < ids.size() && roster.size() < count; ++i) {
        auto it = store.pets.find(ids[i]);
        if (it == store.pets.end()) continue;
        auto pet = createPetFromTemplate(it->second, store.skills, 100);
        if (pet) roster.push_back(std::move(pet));
    }
    return roster;
}

void showRoster(const std::vector<std::unique_ptr<Pet>>& roster) {
    std::cout << "Roster:\n";
    for (std::size_t i = 0; i < roster.size(); ++i) {
        if (!roster[i]) continue;
        std::cout << "  [" << i << "] " << roster[i]->name() << " HP=" << roster[i]->currentHP() << "\n";
    }
}

void printActiveSkills(const Player& player) {
    std::cout << "Active skills:\n";
    int count = 0;
    for (const auto& s : player.activePet().learnedSkills()) {
        if (!s.has_value() || !s->base) continue;
        std::cout << "  skill " << ++count << ": " << s->base->name() << " (id=" << s->base->id()
                  << ", PP=" << s->currentPP << ")\n";
    }
    if (count == 0) {
        std::cout << "  (none)\n";
    }
}

std::optional<ActionData> parseActionInput(const std::string& input, const Player& player) {
    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;
    if (cmd.empty()) return std::nullopt;

    if (cmd == "flee") {
        return ActionData{ ActionType::Flee, 0, 0 };
    }
    if (cmd == "stay") {
        return ActionData{ ActionType::Stay, 0, 0 };
    }
    if (cmd == "switch") {
        std::size_t idx = 0;
        if (!(iss >> idx)) return std::nullopt;
        return ActionData{ ActionType::Switch, 0, idx };
    }

    if (cmd == "skill") {
        int slot = 0;
        if (!(iss >> slot)) return std::nullopt;
        if (slot <= 0) return std::nullopt;
        int count = 0;
        for (const auto& s : player.activePet().learnedSkills()) {
            if (!s.has_value() || !s->base) continue;
            ++count;
            if (count == slot) {
                return ActionData{ ActionType::Skill, s->base->id(), 0 };
            }
        }
        return std::nullopt;
    }

    return std::nullopt;
}

ActionData randomSkillAction(const Player& player, std::mt19937& rng) {
    std::vector<int> skills;
    for (const auto& slot : player.activePet().learnedSkills()) {
        if (slot.has_value() && slot->base) {
            skills.push_back(slot->base->id());
        }
    }
    if (skills.empty()) {
        return ActionData{ ActionType::Stay, 0, 0 };
    }
    std::uniform_int_distribution<std::size_t> dist(0, skills.size() - 1);
    return ActionData{ ActionType::Skill, skills[dist(rng)], 0 };
}

ActionData randomSwitchAction(const std::vector<std::unique_ptr<Pet>>& roster, std::mt19937& rng) {
    std::vector<std::size_t> indices;
    for (std::size_t i = 0; i < roster.size(); ++i) {
        if (roster[i] && !roster[i]->isFainted()) {
            indices.push_back(i);
        }
    }
    if (indices.empty()) {
        return ActionData{ ActionType::Stay, 0, 0 };
    }
    std::uniform_int_distribution<std::size_t> dist(0, indices.size() - 1);
    return ActionData{ ActionType::Switch, 0, indices[dist(rng)] };
}
} // namespace

int runLocalBattle(const std::string& petsDbPath, const std::string& skillsDir) {
    DataStore store;
    std::string error;
    if (!loadDataStore(petsDbPath, skillsDir, store, &error)) {
        std::cerr << "Failed to load data: " << error << "\n";
        return 1;
    }

    std::mt19937 rng{ std::random_device{}() };
    auto roster1 = buildRandomRoster(store, rng, Player::kMaxPets);
    auto roster2 = buildRandomRoster(store, rng, Player::kMaxPets);

    BattleSession session(std::move(roster1), std::move(roster2), store.skills);

    std::cout << "Player roster:\n";
    showRoster(session.roster1());
    std::cout << "AI roster:\n";
    showRoster(session.roster2());

    while (!session.outcome().ended) {
        session.tick();

        PendingType pendingPlayer = session.pendingForPlayer(0);
        if (pendingPlayer == PendingType::ForceSwitch) {
            std::cout << "Your active pet fainted. Choose a replacement within 10s (switch <index>):\n";
            auto line = readLineWithTimeoutCountdown(std::chrono::seconds(10));
            ActionData action;
            bool hasAction = false;
            if (line) {
                auto parsed = parseActionInput(*line, session.player1());
                if (parsed && parsed->type == ActionType::Switch) {
                    action = *parsed;
                    hasAction = true;
                }
            }
            if (!hasAction) {
                action = randomSwitchAction(session.roster1(), rng);
                std::cout << "Timeout. Random switch to index " << action.switchIndex << "\n";
            }
            std::string err;
            if (!session.submitAction(0, action, &err)) {
                std::cout << "Switch failed: " << err << "\n";
            }
        } else if (pendingPlayer == PendingType::ChooseAction) {
            printActiveSkills(session.player1());
            std::cout << "Turn " << session.currentTurn() + 1
                      << " - enter action (skill <1-4> / switch <idx> / flee / stay) within 10s:\n";
            auto line = readLineWithTimeoutCountdown(std::chrono::seconds(10));
            if (!line) {
                ActionData action = randomSkillAction(session.player1(), rng);
                if (action.type == ActionType::Skill) {
                    std::cout << "Timeout. Random skill id " << action.skillId << "\n";
                } else {
                    std::cout << "Timeout. No skills available, staying.\n";
                }
                std::string err;
                if (!session.submitAction(0, action, &err)) {
                    std::cout << "Action rejected: " << err << "\n";
                }
            } else {
                auto parsed = parseActionInput(*line, session.player1());
                if (!parsed) {
                    std::cout << "Invalid action.\n";
                } else {
                    std::string err;
                    if (!session.submitAction(0, *parsed, &err)) {
                        std::cout << "Action rejected: " << err << "\n";
                    }
                }
            }
        }

        PendingType pendingAI = session.pendingForPlayer(1);
        if (pendingAI == PendingType::ForceSwitch) {
            auto action = randomSwitchAction(session.roster2(), rng);
            session.submitAction(1, action);
        } else if (pendingAI == PendingType::ChooseAction) {
            auto action = randomSkillAction(session.player2(), rng);
            session.submitAction(1, action);
        }

        session.tick();

        auto outcome = session.outcome();
        if (outcome.ended) {
            std::cout << "Battle ended. Winner: " << outcome.winner << " Reason: " << outcome.reason << "\n";
            break;
        }

        Pet& p1Pet = session.player1().activePet();
        Pet& p2Pet = session.player2().activePet();
        std::cout << "Player active: " << p1Pet.name() << " HP=" << p1Pet.currentHP() << "/" << p1Pet.maxHP()
                  << "\n";
        std::cout << "AI active: " << p2Pet.name() << " HP=" << p2Pet.currentHP() << "/" << p2Pet.maxHP() << "\n";
    }

    return 0;
}
