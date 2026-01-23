#include "server.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <mutex>
#include <random>
#include <thread>

#include <nlohmann/json.hpp>

#include "battle_session.h"
#include "data_loader.h"
#include "http_server.h"

namespace {
struct ServerState {
    std::mutex mutex;
    DataStore store;
    std::unique_ptr<BattleSession> session;
    std::array<std::string, 2> playerNames{};
    int nextPlayerId = 1;
    bool running = true;
    std::mt19937 rng{ std::random_device{}() };
};

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
        if (pet) {
            roster.push_back(std::move(pet));
        }
    }
    return roster;
}

int parsePlayerId(const std::string& query) {
    auto pos = query.find("playerId=");
    if (pos == std::string::npos) return 0;
    auto val = query.substr(pos + 9);
    try {
        return std::stoi(val);
    } catch (...) {
        return 0;
    }
}

HttpResponse jsonResponse(const nlohmann::json& j, int status = 200) {
    return { status, j.dump(), "application/json" };
}
} // namespace

int runServer(int port, const std::string& petsDbPath, const std::string& skillsDir) {
    ServerState state;
    std::string error;
    if (!loadDataStore(petsDbPath, skillsDir, state.store, &error)) {
        std::cerr << "Failed to load data: " << error << "\n";
        return 1;
    }

    HttpServer server(port);
    server.setHandler([&](const HttpRequest& req) -> HttpResponse {
        std::lock_guard<std::mutex> lock(state.mutex);
        if (req.path == "/join" && req.method == "POST") {
            if (state.nextPlayerId > 2) {
                return jsonResponse({ { "error", "room full" } }, 403);
            }
            nlohmann::json data = nlohmann::json::object();
            if (!req.body.empty()) {
                data = nlohmann::json::parse(req.body, nullptr, false);
            }
            std::string name = data.value("name", "player");
            int assigned = state.nextPlayerId++;
            state.playerNames[assigned - 1] = name;

            if (assigned == 2) {
                auto roster1 = buildRandomRoster(state.store, state.rng, Player::kMaxPets);
                auto roster2 = buildRandomRoster(state.store, state.rng, Player::kMaxPets);
                state.session = std::make_unique<BattleSession>(std::move(roster1), std::move(roster2), state.store.skills);
            }

            return jsonResponse({ { "playerId", assigned }, { "name", name } });
        }

        if (!state.session) {
            return jsonResponse({ { "error", "battle not ready" } }, 400);
        }

        if (req.path == "/state" && req.method == "GET") {
            int playerId = parsePlayerId(req.query);
            if (playerId < 1 || playerId > 2) {
                return jsonResponse({ { "error", "invalid playerId" } }, 400);
            }
            nlohmann::json payload = state.session->stateForPlayer(playerId - 1);
            return jsonResponse(payload);
        }

        if (req.path == "/action" && req.method == "POST") {
            nlohmann::json data = nlohmann::json::parse(req.body, nullptr, false);
            if (data.is_discarded()) {
                return jsonResponse({ { "error", "invalid json" } }, 400);
            }
            int playerId = data.value("playerId", 0);
            if (playerId < 1 || playerId > 2) {
                return jsonResponse({ { "error", "invalid playerId" } }, 400);
            }

            ActionData action;
            std::string type = data.value("type", "stay");
            if (type == "skill") {
                action.type = ActionType::Skill;
                action.skillId = data.value("skillId", 0);
            } else if (type == "switch") {
                action.type = ActionType::Switch;
                action.switchIndex = static_cast<std::size_t>(data.value("index", 0));
            } else if (type == "flee") {
                action.type = ActionType::Flee;
            } else {
                action.type = ActionType::Stay;
            }

            std::string err;
            if (!state.session->submitAction(playerId - 1, action, &err)) {
                return jsonResponse({ { "error", err } }, 400);
            }

            state.session->tick();
            return jsonResponse({ { "status", "ok" } });
        }

        return jsonResponse({ { "error", "not found" } }, 404);
    });

    if (!server.start(&error)) {
        std::cerr << "Failed to start server: " << error << "\n";
        return 1;
    }

    std::thread timer([&]() {
        while (state.running) {
            {
                std::lock_guard<std::mutex> lock(state.mutex);
                if (state.session) {
                    state.session->tick();
                }
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    });

    std::cout << "Server started on port " << port << "\n";
    std::cout << "Press Ctrl+C to stop.\n";

    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    state.running = false;
    timer.join();
    server.stop();
    return 0;
}
