#include "client.h"

#include <chrono>
#include <iostream>
#include <random>
#include <sstream>
#include <thread>

#include <nlohmann/json.hpp>

#include "battle_session.h"
#include "cli_helpers.h"
#include "http_client.h"

namespace {
ActionData actionFromInput(const std::string& input, const nlohmann::json& state, bool forceSwitch) {
    ActionData action{ ActionType::Stay, 0, 0 };
    std::istringstream iss(input);
    std::string cmd;
    iss >> cmd;
    if (forceSwitch) {
        if (cmd == "switch") {
            std::size_t idx = 0;
            if (iss >> idx) {
                action.type = ActionType::Switch;
                action.switchIndex = idx;
            }
        }
        return action;
    }

    if (cmd == "skill") {
        int slot = 0;
        if (iss >> slot) {
            if (state.contains("self")) {
                auto pets = state["self"]["pets"];
                std::size_t activeIndex = state["self"]["activeIndex"].get<std::size_t>();
                for (const auto& pet : pets) {
                    if (pet["index"].get<std::size_t>() != activeIndex) continue;
                    int count = 0;
                    for (const auto& skill : pet["skills"]) {
                        ++count;
                        if (count == slot) {
                            action.type = ActionType::Skill;
                            action.skillId = skill["id"].get<int>();
                            return action;
                        }
                    }
                }
            }
        }
    } else if (cmd == "switch") {
        std::size_t idx = 0;
        if (iss >> idx) {
            action.type = ActionType::Switch;
            action.switchIndex = idx;
        }
    } else if (cmd == "flee") {
        action.type = ActionType::Flee;
    } else if (cmd == "stay") {
        action.type = ActionType::Stay;
    }

    return action;
}

ActionData randomSkillFromState(const nlohmann::json& state) {
    ActionData action{ ActionType::Stay, 0, 0 };
    if (!state.contains("self")) return action;

    auto self = state["self"];
    std::size_t activeIndex = self["activeIndex"].get<std::size_t>();
    std::vector<int> skills;
    for (const auto& pet : self["pets"]) {
        if (pet["index"].get<std::size_t>() != activeIndex) continue;
        for (const auto& skill : pet["skills"]) {
            skills.push_back(skill.value("id", 0));
        }
        break;
    }
    if (skills.empty()) return action;

    static thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<std::size_t> dist(0, skills.size() - 1);
    action.type = ActionType::Skill;
    action.skillId = skills[dist(rng)];
    return action;
}

void printState(const nlohmann::json& state) {
    if (!state.contains("self")) return;
    std::cout << "Turn " << state.value("turn", 0) + 1 << "\n";
    auto self = state["self"];
    std::size_t activeIndex = self["activeIndex"].get<std::size_t>();
    for (const auto& pet : self["pets"]) {
        std::size_t idx = pet["index"].get<std::size_t>();
        std::cout << "  [" << idx << "] " << pet.value("name", "") << " HP=" << pet.value("hp", 0)
                  << "/" << pet.value("maxHp", 0);
        if (idx == activeIndex) {
            std::cout << " (active)";
        }
        std::cout << "\n";
        if (idx == activeIndex) {
            int s = 0;
            for (const auto& skill : pet["skills"]) {
                std::cout << "    skill " << ++s << ": " << skill.value("name", "")
                          << " (id=" << skill.value("id", 0) << ", PP=" << skill.value("pp", 0) << ")\n";
            }
        }
    }
}
} // namespace

int runClient(const std::string& host, int port) {
    HttpClient client(host, port);

    std::string name;
    std::cout << "Enter your name: ";
    std::getline(std::cin, name);

    auto joinResp = client.post("/join", { { "name", name } });
    if (joinResp.status != 200) {
        std::cerr << "Join failed: " << joinResp.body << "\n";
        return 1;
    }

    nlohmann::json joinJson = nlohmann::json::parse(joinResp.body, nullptr, false);
    if (joinJson.is_discarded()) {
        std::cerr << "Join response invalid.\n";
        return 1;
    }

    int playerId = joinJson.value("playerId", 0);
    if (playerId <= 0) {
        std::cerr << "Join failed: " << joinResp.body << "\n";
        return 1;
    }

    std::cout << "Joined as player " << playerId << "\n";

    while (true) {
        auto stateResp = client.get("/state?playerId=" + std::to_string(playerId));
        if (stateResp.status != 200) {
            std::cerr << "State error: " << stateResp.body << "\n";
            std::this_thread::sleep_for(std::chrono::milliseconds(500));
            continue;
        }

        nlohmann::json state = nlohmann::json::parse(stateResp.body, nullptr, false);
        if (state.is_discarded()) {
            std::cerr << "Invalid state response.\n";
            continue;
        }

        if (state.value("battleOver", false)) {
            std::cout << "Battle ended. Winner=" << state.value("winner", 0)
                      << " Reason=" << state.value("reason", "") << "\n";
            break;
        }

        std::string pending = state.value("pending", "wait");
        if (pending == "wait") {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            continue;
        }

        printState(state);

        if (pending == "force_switch") {
            std::cout << "Force switch: enter 'switch <index>' within 10s:\n";
            auto line = readLineWithTimeoutCountdown(std::chrono::seconds(10));
            std::string input = line.value_or("switch 0");
            ActionData action = actionFromInput(input, state, true);

            nlohmann::json payload = { { "playerId", playerId }, { "type", "switch" },
                                       { "index", action.switchIndex } };
            client.post("/action", payload);
            continue;
        }

        std::cout << "Action (skill <1-4> / switch <idx> / flee / stay) within 10s:\n";
        auto line = readLineWithTimeoutCountdown(std::chrono::seconds(10));
        ActionData action;
        if (!line) {
            action = randomSkillFromState(state);
            if (action.type == ActionType::Skill) {
                std::cout << "Timeout. Random skill id " << action.skillId << "\n";
            } else {
                std::cout << "Timeout. No skills available, staying.\n";
            }
        } else {
            action = actionFromInput(*line, state, false);
        }

        nlohmann::json payload;
        payload["playerId"] = playerId;
        switch (action.type) {
            case ActionType::Skill:
                payload["type"] = "skill";
                payload["skillId"] = action.skillId;
                break;
            case ActionType::Switch:
                payload["type"] = "switch";
                payload["index"] = action.switchIndex;
                break;
            case ActionType::Flee:
                payload["type"] = "flee";
                break;
            case ActionType::Stay:
            default:
                payload["type"] = "stay";
                break;
        }

        auto actionResp = client.post("/action", payload);
        if (actionResp.status != 200) {
            std::cerr << "Action error: " << actionResp.body << "\n";
        }
    }

    return 0;
}
