#include "server.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <thread>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <nlohmann/json.hpp>

#include "battle_session.h"
#include "data_loader.h"
#include "http_server.h"

namespace {
using Clock = std::chrono::steady_clock;
constexpr std::chrono::seconds kParticipantTimeout{60};

struct PlayerSlot {
    bool occupied = false;
    std::string name;
    bool ready = false;
};

struct Room {
    std::string name;
    std::array<PlayerSlot, 2> players{};
    std::vector<std::string> spectators;
    std::array<std::optional<Clock::time_point>, 2> playerSeen{};
    std::unordered_map<std::string, Clock::time_point> spectatorSeen;
    std::unique_ptr<BattleSession> session;
};

struct ServerState {
    std::mutex mutex;
    DataStore store;
    std::unordered_map<std::string, Room> rooms;
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

std::string queryValue(const std::string& query, const std::string& key) {
    std::string needle = key + "=";
    auto pos = query.find(needle);
    if (pos == std::string::npos) return {};
    auto start = pos + needle.size();
    auto end = query.find('&', start);
    return query.substr(start, end == std::string::npos ? std::string::npos : end - start);
}

int parsePlayerId(const std::string& query) {
    auto val = queryValue(query, "playerId");
    if (val.empty()) return 0;
    try {
        return std::stoi(val);
    } catch (...) {
        return 0;
    }
}

HttpResponse jsonResponse(const nlohmann::json& j, int status = 200) {
    return { status, j.dump(), "application/json" };
}

bool roomHasName(const Room& room, const std::string& name) {
    if (name.empty()) return false;
    for (const auto& slot : room.players) {
        if (slot.occupied && slot.name == name) return true;
    }
    for (const auto& spec : room.spectators) {
        if (spec == name) return true;
    }
    return false;
}

int addPlayer(Room& room, const std::string& name) {
    for (std::size_t i = 0; i < room.players.size(); ++i) {
        if (!room.players[i].occupied) {
            room.players[i].occupied = true;
            room.players[i].name = name;
            room.players[i].ready = false;
            return static_cast<int>(i) + 1;
        }
    }
    return 0;
}

void removePlayer(Room& room, int playerId) {
    if (playerId < 1 || playerId > 2) return;
    std::size_t idx = static_cast<std::size_t>(playerId - 1);
    auto& slot = room.players[idx];
    slot.occupied = false;
    slot.name.clear();
    slot.ready = false;
    room.playerSeen[idx].reset();
}

void removeSpectator(Room& room, const std::string& name) {
    room.spectatorSeen.erase(name);
    room.spectators.erase(std::remove(room.spectators.begin(), room.spectators.end(), name), room.spectators.end());
}

bool roomFull(const Room& room) {
    return room.players[0].occupied && room.players[1].occupied;
}

bool roomEmpty(const Room& room) {
    if (room.players[0].occupied || room.players[1].occupied) return false;
    return room.spectators.empty();
}

bool roomReady(const Room& room) {
    return room.players[0].occupied && room.players[1].occupied && room.players[0].ready && room.players[1].ready;
}

nlohmann::json roomSummary(const Room& room) {
    nlohmann::json j;
    j["name"] = room.name;
    j["spectators"] = static_cast<int>(room.spectators.size());
    j["spectatorNames"] = room.spectators;
    j["battleStarted"] = room.session != nullptr;
    j["battleOver"] = room.session ? room.session->outcome().ended : false;
    j["winner"] = room.session ? room.session->outcome().winner : 0;
    j["reason"] = room.session ? room.session->outcome().reason : "";
    j["players"] = nlohmann::json::array();
    for (int i = 0; i < 2; ++i) {
        nlohmann::json p;
        p["id"] = i + 1;
        p["present"] = room.players[i].occupied;
        p["name"] = room.players[i].name;
        p["ready"] = room.players[i].ready;
        j["players"].push_back(std::move(p));
    }
    return j;
}

void touchPlayer(Room& room, int playerId, Clock::time_point now) {
    if (playerId < 1 || playerId > 2) return;
    room.playerSeen[static_cast<std::size_t>(playerId - 1)] = now;
}

void touchSpectator(Room& room, const std::string& name, Clock::time_point now) {
    if (name.empty()) return;
    room.spectatorSeen[name] = now;
}

void expireRoomParticipants(Room& room, Clock::time_point now) {
    for (int i = 0; i < 2; ++i) {
        if (room.playerSeen[i].has_value() && (now - *room.playerSeen[i]) > kParticipantTimeout) {
            removePlayer(room, i + 1);
            if (room.session && !room.session->outcome().ended) {
                room.session->forfeit(i);
            }
        }
    }

    for (auto it = room.spectatorSeen.begin(); it != room.spectatorSeen.end();) {
        if ((now - it->second) > kParticipantTimeout) {
            std::string name = it->first;
            it = room.spectatorSeen.erase(it);
            removeSpectator(room, name);
            continue;
        }
        ++it;
    }
}

void syncSpectators(Room& room) {
    std::unordered_set<std::string> seen;
    for (const auto& kv : room.spectatorSeen) {
        seen.insert(kv.first);
    }
    room.spectators.erase(std::remove_if(room.spectators.begin(), room.spectators.end(),
                                         [&](const std::string& name) { return seen.find(name) == seen.end(); }),
                          room.spectators.end());
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
        auto now = Clock::now();
        if (req.path == "/rooms" && req.method == "GET") {
            nlohmann::json list = nlohmann::json::array();
            for (auto& kv : state.rooms) {
                syncSpectators(kv.second);
                list.push_back(roomSummary(kv.second));
            }
            return jsonResponse({ { "rooms", list } });
        }

        if (req.path == "/room" && req.method == "GET") {
            std::string roomName = queryValue(req.query, "name");
            if (roomName.empty()) {
                return jsonResponse({ { "error", "missing room name" } }, 400);
            }
            auto it = state.rooms.find(roomName);
            if (it == state.rooms.end()) {
                return jsonResponse({ { "error", "room not found" } }, 404);
            }
            syncSpectators(it->second);
            return jsonResponse(roomSummary(it->second));
        }

        if (req.path == "/create" && req.method == "POST") {
            nlohmann::json data = nlohmann::json::parse(req.body, nullptr, false);
            if (data.is_discarded()) {
                return jsonResponse({ { "error", "invalid json" } }, 400);
            }
            std::string roomName = data.value("room", "");
            std::string name = data.value("name", "player");
            if (roomName.empty()) {
                return jsonResponse({ { "error", "room name required" } }, 400);
            }
            if (state.rooms.find(roomName) != state.rooms.end()) {
                return jsonResponse({ { "error", "room exists" } }, 409);
            }
            Room room;
            room.name = roomName;
            if (roomHasName(room, name)) {
                return jsonResponse({ { "error", "name taken" } }, 409);
            }
            int assigned = addPlayer(room, name);
            if (assigned == 0) {
                return jsonResponse({ { "error", "room full" } }, 403);
            }
            touchPlayer(room, assigned, now);
            state.rooms.emplace(roomName, std::move(room));
            return jsonResponse({ { "room", roomName }, { "playerId", assigned }, { "name", name } });
        }

        if (req.path == "/join" && req.method == "POST") {
            nlohmann::json data = nlohmann::json::parse(req.body, nullptr, false);
            if (data.is_discarded()) {
                return jsonResponse({ { "error", "invalid json" } }, 400);
            }
            std::string roomName = data.value("room", "");
            std::string name = data.value("name", "player");
            auto it = state.rooms.find(roomName);
            if (it == state.rooms.end()) {
                return jsonResponse({ { "error", "room not found" } }, 404);
            }
            Room& room = it->second;
            if (roomHasName(room, name)) {
                return jsonResponse({ { "error", "name taken" } }, 409);
            }
            if (roomFull(room)) {
                return jsonResponse({ { "error", "room full" } }, 403);
            }
            if (room.session) {
                return jsonResponse({ { "error", "battle already started" } }, 403);
            }
            int assigned = addPlayer(room, name);
            touchPlayer(room, assigned, now);
            return jsonResponse({ { "room", roomName }, { "playerId", assigned }, { "name", name } });
        }

        if (req.path == "/join_random" && req.method == "POST") {
            nlohmann::json data = nlohmann::json::parse(req.body, nullptr, false);
            if (data.is_discarded()) {
                return jsonResponse({ { "error", "invalid json" } }, 400);
            }
            std::string name = data.value("name", "player");
            for (auto& kv : state.rooms) {
                Room& room = kv.second;
                if (room.session) continue;
                if (roomFull(room)) continue;
                if (roomHasName(room, name)) continue;
                int assigned = addPlayer(room, name);
                if (assigned > 0) {
                    touchPlayer(room, assigned, now);
                    return jsonResponse({ { "room", room.name }, { "playerId", assigned }, { "name", name } });
                }
            }
            return jsonResponse({ { "error", "no available rooms" } }, 404);
        }

        if (req.path == "/spectate" && req.method == "POST") {
            nlohmann::json data = nlohmann::json::parse(req.body, nullptr, false);
            if (data.is_discarded()) {
                return jsonResponse({ { "error", "invalid json" } }, 400);
            }
            std::string roomName = data.value("room", "");
            std::string name = data.value("name", "spectator");
            auto it = state.rooms.find(roomName);
            if (it == state.rooms.end()) {
                return jsonResponse({ { "error", "room not found" } }, 404);
            }
            Room& room = it->second;
            if (roomHasName(room, name)) {
                return jsonResponse({ { "error", "name taken" } }, 409);
            }
            room.spectators.push_back(name);
            touchSpectator(room, name, now);
            return jsonResponse({ { "room", roomName }, { "name", name }, { "spectator", true } });
        }

        if (req.path == "/ready" && req.method == "POST") {
            nlohmann::json data = nlohmann::json::parse(req.body, nullptr, false);
            if (data.is_discarded()) {
                return jsonResponse({ { "error", "invalid json" } }, 400);
            }
            std::string roomName = data.value("room", "");
            int playerId = data.value("playerId", 0);
            bool ready = data.value("ready", true);
            auto it = state.rooms.find(roomName);
            if (it == state.rooms.end()) {
                return jsonResponse({ { "error", "room not found" } }, 404);
            }
            if (playerId < 1 || playerId > 2) {
                return jsonResponse({ { "error", "invalid playerId" } }, 400);
            }
            Room& room = it->second;
            if (!room.players[playerId - 1].occupied) {
                return jsonResponse({ { "error", "player not in room" } }, 403);
            }
            room.players[playerId - 1].ready = ready;
            touchPlayer(room, playerId, now);
            if (roomReady(room) && !room.session) {
                auto roster1 = buildRandomRoster(state.store, state.rng, Player::kMaxPets);
                auto roster2 = buildRandomRoster(state.store, state.rng, Player::kMaxPets);
                room.session = std::make_unique<BattleSession>(std::move(roster1), std::move(roster2), state.store.skills);
            }
            return jsonResponse({ { "status", "ok" }, { "battleStarted", room.session != nullptr } });
        }

        if (req.path == "/leave" && req.method == "POST") {
            nlohmann::json data = nlohmann::json::parse(req.body, nullptr, false);
            if (data.is_discarded()) {
                return jsonResponse({ { "error", "invalid json" } }, 400);
            }
            std::string roomName = data.value("room", "");
            int playerId = data.value("playerId", 0);
            std::string name = data.value("name", "");
            auto it = state.rooms.find(roomName);
            if (it == state.rooms.end()) {
                return jsonResponse({ { "error", "room not found" } }, 404);
            }
            Room& room = it->second;
            if (playerId >= 1 && playerId <= 2) {
                if (room.session && !room.session->outcome().ended) {
                    room.session->forfeit(playerId - 1);
                }
                removePlayer(room, playerId);
            } else if (!name.empty()) {
                removeSpectator(room, name);
            }
            if (room.session && room.session->outcome().ended && roomEmpty(room)) {
                state.rooms.erase(it);
                return jsonResponse({ { "status", "ok" } });
            }
            if (!room.session && roomEmpty(room)) {
                state.rooms.erase(it);
                return jsonResponse({ { "status", "ok" } });
            }
            return jsonResponse({ { "status", "ok" } });
        }

        if (req.path == "/state" && req.method == "GET") {
            std::string roomName = queryValue(req.query, "room");
            auto it = state.rooms.find(roomName);
            if (it == state.rooms.end()) {
                return jsonResponse({ { "error", "room not found" } }, 404);
            }
            Room& room = it->second;
            std::string spectatorFlag = queryValue(req.query, "spectator");
            bool spectator = (spectatorFlag == "1" || spectatorFlag == "true");
            if (!room.session) {
                return jsonResponse({ { "status", "waiting" }, { "room", roomSummary(room) } });
            }
            if (spectator) {
                std::string specName = queryValue(req.query, "name");
                touchSpectator(room, specName, now);
                nlohmann::json payload = room.session->spectatorState();
                payload["room"] = roomSummary(room);
                return jsonResponse(payload);
            }
            int playerId = parsePlayerId(req.query);
            if (playerId < 1 || playerId > 2) {
                return jsonResponse({ { "error", "invalid playerId" } }, 400);
            }
            touchPlayer(room, playerId, now);
            nlohmann::json payload = room.session->stateForPlayer(playerId - 1);
            payload["room"] = roomSummary(room);
            return jsonResponse(payload);
        }

        if (req.path == "/action" && req.method == "POST") {
            nlohmann::json data = nlohmann::json::parse(req.body, nullptr, false);
            if (data.is_discarded()) {
                return jsonResponse({ { "error", "invalid json" } }, 400);
            }
            std::string roomName = data.value("room", "");
            int playerId = data.value("playerId", 0);
            auto it = state.rooms.find(roomName);
            if (it == state.rooms.end()) {
                return jsonResponse({ { "error", "room not found" } }, 404);
            }
            Room& room = it->second;
            if (!room.session) {
                return jsonResponse({ { "error", "battle not ready" } }, 400);
            }
            if (playerId < 1 || playerId > 2) {
                return jsonResponse({ { "error", "invalid playerId" } }, 400);
            }
            touchPlayer(room, playerId, now);

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
            if (!room.session->submitAction(playerId - 1, action, &err)) {
                return jsonResponse({ { "error", err } }, 400);
            }

            room.session->tick();
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
                for (auto it = state.rooms.begin(); it != state.rooms.end();) {
                    Room& room = it->second;
                    expireRoomParticipants(room, Clock::now());
                    if (!room.session && roomEmpty(room)) {
                        it = state.rooms.erase(it);
                        continue;
                    }
                    if (room.session) {
                        room.session->tick();
                        if (room.session->outcome().ended && roomEmpty(room)) {
                            it = state.rooms.erase(it);
                            continue;
                        }
                    }
                    ++it;
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
