#include "client.h"

#include <algorithm>
#include <chrono>
#include <iostream>
#include <optional>
#include <random>
#include <sstream>
#include <thread>
#include <unordered_set>
#include <vector>
#include <unordered_map>

#ifndef _WIN32
#include <termios.h>
#include <unistd.h>
#endif

#include <nlohmann/json.hpp>

#include "battle_session.h"
#include "cli_helpers.h"
#include "http_client.h"

namespace {
constexpr const char *kRed = "\033[31m";
constexpr const char *kReset = "\033[0m";

std::string promptLine(const std::string &prompt) {
  std::cout << prompt;
  std::string line;
  std::getline(std::cin, line);
  return line;
}

#ifndef _WIN32
struct TermiosGuard {
  termios old{};
  bool enabled = false;
};

bool enableRawMode(TermiosGuard &guard) {
  if (!isatty(STDIN_FILENO)) return false;
  if (tcgetattr(STDIN_FILENO, &guard.old) != 0) return false;
  termios raw = guard.old;
  raw.c_lflag &= static_cast<unsigned int>(~(ICANON | ECHO));
  raw.c_cc[VMIN] = 1;
  raw.c_cc[VTIME] = 0;
  if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) != 0) return false;
  guard.enabled = true;
  return true;
}

void disableRawMode(TermiosGuard &guard) {
  if (guard.enabled) {
    tcsetattr(STDIN_FILENO, TCSANOW, &guard.old);
    guard.enabled = false;
  }
}

int readChar() {
  unsigned char c = 0;
  if (read(STDIN_FILENO, &c, 1) == 1) return static_cast<int>(c);
  return -1;
}

int readCharWithTimeout(int timeoutMs) {
  if (timeoutMs <= 0) return readChar();
  fd_set readfds;
  FD_ZERO(&readfds);
  FD_SET(STDIN_FILENO, &readfds);
  timeval tv{};
  tv.tv_sec = timeoutMs / 1000;
  tv.tv_usec = (timeoutMs % 1000) * 1000;
  int ret = select(STDIN_FILENO + 1, &readfds, nullptr, nullptr, &tv);
  if (ret <= 0) return -1;
  return readChar();
}

void enterAltScreen() {
  std::cout << "\033[?1049h\033[H\033[2J" << std::flush;
}

void exitAltScreen() {
  std::cout << "\033[?1049l" << std::flush;
}
#endif

int selectMenu(const std::string &title, const std::vector<std::string> &options, int initial = 0) {
  if (options.empty()) return -1;
#ifndef _WIN32
  TermiosGuard guard;
  if (!enableRawMode(guard)) {
    std::cout << title << "\n";
    for (std::size_t i = 0; i < options.size(); ++i) {
      std::cout << "  [" << i + 1 << "] " << options[i] << "\n";
    }
    std::string line = promptLine("Select number (or empty to cancel): ");
    if (line.empty()) return -1;
    try {
      int idx = std::stoi(line);
      if (idx < 1 || static_cast<std::size_t>(idx) > options.size()) return -1;
      return idx - 1;
    } catch (...) {
      return -1;
    }
  }

  int selected = std::min<int>(initial, static_cast<int>(options.size() - 1));
  auto render = [&]() {
    std::cout << title << "\n";
    for (std::size_t i = 0; i < options.size(); ++i) {
      std::cout << (static_cast<int>(i) == selected ? "> " : "  ") << options[i] << "\n";
    }
    std::cout << std::flush;
  };

  render();
  while (true) {
    int c = readChar();
    if (c == '\n' || c == '\r') {
      disableRawMode(guard);
      return selected;
    }
    if (c == 'q') {
      disableRawMode(guard);
      return -1;
    }
    if (c == 27) {
      int c1 = readChar();
      int c2 = readChar();
      if (c1 == '[' && c2 == 'A') {
        selected = (selected - 1 + static_cast<int>(options.size())) % static_cast<int>(options.size());
      } else if (c1 == '[' && c2 == 'B') {
        selected = (selected + 1) % static_cast<int>(options.size());
      }
      for (std::size_t i = 0; i < options.size() + 1; ++i) {
        std::cout << "\033[1A";
      }
      for (std::size_t i = 0; i < options.size() + 1; ++i) {
        std::cout << "\r\033[2K";
        if (i == 0) {
          std::cout << title;
        } else {
          std::size_t optIdx = i - 1;
          std::cout << (static_cast<int>(optIdx) == selected ? "> " : "  ") << options[optIdx];
        }
        std::cout << "\n";
      }
      std::cout << std::flush;
      continue;
    }
    if (c == 'k') {
      selected = (selected - 1 + static_cast<int>(options.size())) % static_cast<int>(options.size());
    } else if (c == 'j') {
      selected = (selected + 1) % static_cast<int>(options.size());
    }
    for (std::size_t i = 0; i < options.size() + 1; ++i) {
      std::cout << "\033[1A";
    }
    for (std::size_t i = 0; i < options.size() + 1; ++i) {
      std::cout << "\r\033[2K";
      if (i == 0) {
        std::cout << title;
      } else {
        std::size_t optIdx = i - 1;
        std::cout << (static_cast<int>(optIdx) == selected ? "> " : "  ") << options[optIdx];
      }
      std::cout << "\n";
    }
    std::cout << std::flush;
  }
#else
  std::cout << title << "\n";
  for (std::size_t i = 0; i < options.size(); ++i) {
    std::cout << "  [" << i + 1 << "] " << options[i] << "\n";
  }
  std::string line = promptLine("Select number (or empty to cancel): ");
  if (line.empty()) return -1;
  try {
    int idx = std::stoi(line);
    if (idx < 1 || static_cast<std::size_t>(idx) > options.size()) return -1;
    return idx - 1;
  } catch (...) {
    return -1;
  }
#endif
}

enum class NavKey { None, Up, Down, Left, Right, Enter, Quit };

NavKey readNavKey(int timeoutMs) {
#ifndef _WIN32
  int c = readCharWithTimeout(timeoutMs);
  if (c < 0) return NavKey::None;
  if (c == '\n' || c == '\r') return NavKey::Enter;
  if (c == 'q') return NavKey::Quit;
  if (c == 'k') return NavKey::Up;
  if (c == 'j') return NavKey::Down;
  if (c == 'h') return NavKey::Left;
  if (c == 'l') return NavKey::Right;
  if (c == 27) {
    int c1 = readCharWithTimeout(10);
    int c2 = readCharWithTimeout(10);
    if (c1 == '[' && c2 == 'A') return NavKey::Up;
    if (c1 == '[' && c2 == 'B') return NavKey::Down;
    if (c1 == '[' && c2 == 'C') return NavKey::Right;
    if (c1 == '[' && c2 == 'D') return NavKey::Left;
  }
  return NavKey::None;
#else
  (void)timeoutMs;
  return NavKey::None;
#endif
}

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

void printRoomInfo(const nlohmann::json &room) {
  std::cout << "Room: " << room.value("name", "") << "\n";
  if (room.contains("players")) {
    for (const auto &p : room["players"]) {
      int id = p.value("id", 0);
      bool present = p.value("present", false);
      std::string name = p.value("name", "");
      bool ready = p.value("ready", false);
      std::cout << "  Player " << id << ": ";
      if (present) {
        std::cout << name << (ready ? " [ready]" : " [not ready]");
      } else {
        std::cout << "(empty)";
      }
      std::cout << "\n";
    }
  }
  std::cout << "  Spectators: " << room.value("spectators", 0);
  if (room.contains("spectatorNames") && room["spectatorNames"].is_array()) {
    std::cout << " [";
    bool first = true;
    for (const auto &s : room["spectatorNames"]) {
      if (!first)
        std::cout << ", ";
      std::cout << s.get<std::string>();
      first = false;
    }
    std::cout << "]";
  }
  std::cout << "\n";
}

std::unordered_map<std::string, std::string> collectPlayerRoles(const nlohmann::json &room) {
  std::unordered_map<std::string, std::string> out;
  if (room.contains("players")) {
    for (const auto &p : room["players"]) {
      if (p.value("present", false)) {
        std::string name = p.value("name", "");
        int id = p.value("id", 0);
        if (!name.empty()) {
          std::string role = id == 1 ? "玩家1" : (id == 2 ? "玩家2" : "玩家");
          out[name] = role;
        }
      }
    }
  }
  return out;
}

void collectSpectatorNames(const nlohmann::json &room, std::unordered_set<std::string> &out) {
  out.clear();
  if (room.contains("spectatorNames") && room["spectatorNames"].is_array()) {
    for (const auto &s : room["spectatorNames"]) {
      out.insert(s.get<std::string>());
    }
  }
}

void renderRoomScreen(const nlohmann::json &room,
                      const std::vector<std::string> &messages,
                      const std::vector<std::string> &options,
                      int selected) {
  std::cout << "\033[H\033[2J";
  printRoomInfo(room);
  for (const auto &msg : messages) {
    std::cout << msg << "\n";
  }
  std::cout << "\nActions:\n";
  for (std::size_t i = 0; i < options.size(); ++i) {
    std::cout << (static_cast<int>(i) == selected ? "> " : "  ") << options[i] << "\n";
  }
  std::cout << std::flush;
}

struct LeftItem {
  std::string label;
  bool selectable = false;
  int skillId = 0;
  std::size_t switchIndex = 0;
};

void renderBattleInfo(const nlohmann::json &state, int remaining) {
  std::cout << "Turn " << state.value("turn", 0) + 1;
  if (remaining >= 0) {
    std::cout << "  Time left: " << remaining << "s";
  }
  std::cout << "\n";
  if (!state.contains("self"))
    return;
  auto self = state["self"];
  std::size_t activeIndex = self["activeIndex"].get<std::size_t>();
  for (const auto &pet : self["pets"]) {
    std::size_t idx = pet["index"].get<std::size_t>();
    std::cout << "  [" << idx << "] " << pet.value("name", "")
              << " HP=" << pet.value("hp", 0) << "/" << pet.value("maxHp", 0);
    if (idx == activeIndex)
      std::cout << " (active)";
    if (pet.value("fainted", false))
      std::cout << " (fainted)";
    std::cout << "\n";
    if (idx == activeIndex) {
      int s = 0;
      for (const auto &skill : pet["skills"]) {
        std::cout << "    skill " << ++s << ": " << skill.value("name", "")
                  << " (id=" << skill.value("id", 0)
                  << ", PP=" << skill.value("pp", 0) << ")\n";
      }
      for (; s < 4; ++s) {
        std::cout << "    skill " << (s + 1) << ": (empty)\n";
      }
    }
  }
  if (state.contains("opponent")) {
    auto opp = state["opponent"];
    std::cout << "Opponent active: "
              << opp["active"].value("name", "")
              << " HP=" << opp["active"].value("hp", 0) << "/"
              << opp["active"].value("maxHp", 0) << "\n";
  }
}

std::vector<LeftItem> buildSkillItems(const nlohmann::json &state) {
  std::vector<LeftItem> items;
  if (!state.contains("self"))
    return items;
  auto self = state["self"];
  std::size_t activeIndex = self["activeIndex"].get<std::size_t>();
  for (const auto &pet : self["pets"]) {
    if (pet["index"].get<std::size_t>() != activeIndex)
      continue;
    int s = 0;
    for (const auto &skill : pet["skills"]) {
      LeftItem it;
      it.selectable = true;
      it.skillId = skill.value("id", 0);
      std::ostringstream oss;
      oss << (s + 1) << ") " << skill.value("name", "") << " (PP "
          << skill.value("pp", 0) << "/" << skill.value("maxPP", 0) << ")";
      it.label = oss.str();
      items.push_back(it);
      ++s;
      if (s >= 4)
        break;
    }
    for (; s < 4; ++s) {
      LeftItem it;
      it.selectable = false;
      it.label = std::to_string(s + 1) + ") (empty)";
      items.push_back(it);
    }
    break;
  }
  return items;
}

std::vector<LeftItem> buildSwitchItems(const nlohmann::json &state) {
  std::vector<LeftItem> items;
  if (!state.contains("self"))
    return items;
  auto self = state["self"];
  std::size_t activeIndex = self["activeIndex"].get<std::size_t>();
  for (const auto &pet : self["pets"]) {
    LeftItem it;
    std::size_t idx = pet["index"].get<std::size_t>();
    bool fainted = pet.value("fainted", false);
    bool active = idx == activeIndex;
    it.switchIndex = idx;
    it.selectable = !fainted && !active;
    std::ostringstream oss;
    if (fainted) {
      oss << "\033[9m";
    }
    if (active) {
      oss << "\033[31m";
    }
    oss << "[" << idx << "] " << pet.value("name", "")
        << " HP=" << pet.value("hp", 0) << "/"
        << pet.value("maxHp", 0);
    if (fainted || active) {
      oss << "\033[0m";
    }
    it.label = oss.str();
    items.push_back(it);
  }
  return items;
}

void renderBattleMenu(const std::vector<LeftItem> &left,
                      const std::vector<std::string> &right,
                      int leftIdx,
                      int rightIdx,
                      bool focusLeft,
                      const std::string &message) {
  std::cout << "\n";
  if (!message.empty()) {
    std::cout << message << "\n\n";
  }
  std::size_t rows = std::max(left.size(), right.size());
  for (std::size_t i = 0; i < rows; ++i) {
    if (i < left.size()) {
      const auto &li = left[i];
      std::cout << (focusLeft && static_cast<int>(i) == leftIdx ? "> " : "  ")
                << li.label;
    }
    if (i < right.size()) {
      if (i < left.size()) {
        std::cout << std::string(2, ' ');
      }
      std::cout << ( !focusLeft && static_cast<int>(i) == rightIdx ? "> " : "  ")
                << right[i];
    }
    std::cout << "\n";
  }
  std::cout << std::flush;
}

void printRooms(const nlohmann::json &rooms) {
  if (!rooms.is_array() || rooms.empty()) {
    std::cout << "No rooms.\n";
    return;
  }
  for (const auto &room : rooms) {
    std::string name = room.value("name", "");
    bool started = room.value("battleStarted", false);
    int spectators = room.value("spectators", 0);
    int readyCount = 0;
    int presentCount = 0;
    if (room.contains("players")) {
      for (const auto &p : room["players"]) {
        if (p.value("present", false)) {
          ++presentCount;
          if (p.value("ready", false))
            ++readyCount;
        }
      }
    }
    std::cout << "- " << name << " players " << presentCount << "/2"
              << " ready " << readyCount << "/2"
              << " spectators " << spectators << (started ? " [battle]" : "")
              << "\n";
  }
}

std::optional<std::string> selectRoomFromList(const nlohmann::json &rooms, const std::string &title) {
  if (!rooms.is_array() || rooms.empty()) {
    std::cout << "No rooms.\n";
    return std::nullopt;
  }
  std::vector<std::string> labels;
  std::vector<std::string> names;
  for (const auto &room : rooms) {
    std::string name = room.value("name", "");
    bool started = room.value("battleStarted", false);
    int spectators = room.value("spectators", 0);
    int readyCount = 0;
    int presentCount = 0;
    if (room.contains("players")) {
      for (const auto &p : room["players"]) {
        if (p.value("present", false)) {
          ++presentCount;
          if (p.value("ready", false))
            ++readyCount;
        }
      }
    }
    std::ostringstream oss;
    oss << name << " players " << presentCount << "/2"
        << " ready " << readyCount << "/2"
        << " spectators " << spectators
        << (started ? " [battle]" : "");
    labels.push_back(oss.str());
    names.push_back(name);
  }
  int idx = selectMenu(title, labels, 0);
  if (idx < 0 || static_cast<std::size_t>(idx) >= names.size())
    return std::nullopt;
  return names[static_cast<std::size_t>(idx)];
}

void printLastActions(const nlohmann::json &lastActions) {
  if (!lastActions.is_object() || lastActions.empty())
    return;
  int turn = lastActions.value("turn", 0);
  auto printAction = [&](const nlohmann::json &act) {
    std::string type = act.value("type", "stay");
    if (type == "skill") {
      std::cout << "skill(" << act.value("skillId", 0) << ")";
    } else if (type == "switch") {
      std::cout << "switch(" << act.value("index", 0) << ")";
    } else {
      std::cout << type;
    }
  };
  std::cout << "Turn " << turn << " actions: ";
  std::cout << "P1 ";
  printAction(lastActions.value("p1", nlohmann::json::object()));
  std::cout << " | P2 ";
  printAction(lastActions.value("p2", nlohmann::json::object()));
  std::cout << "\n";
}

int runBattle(HttpClient &client, const std::string &room, int playerId) {
#ifndef _WIN32
  TermiosGuard guard;
  if (!enableRawMode(guard)) {
    std::cerr << "Terminal raw mode unavailable.\n";
    return 1;
  }
  enterAltScreen();

  std::string pending;
  std::string lastPending;
  auto actionDeadline = std::chrono::steady_clock::now();
  bool actionActive = false;
  bool actionSubmitted = false;
  int rightIdx = 0;
  int leftIdx = 0;
  bool focusLeft = false;
  std::string message;
  int lastRemaining = -1;
  nlohmann::json state = nlohmann::json::object();
  auto lastFetch = std::chrono::steady_clock::now() - std::chrono::seconds(1);

  std::vector<std::string> rightItems = {"技能", "更换宠物", "逃跑", "战术挂机"};

  auto chooseAutoSwitch = [&](const nlohmann::json &s) -> std::size_t {
    if (!s.contains("self"))
      return 0;
    auto self = s["self"];
    std::size_t activeIndex = self["activeIndex"].get<std::size_t>();
    for (const auto &pet : self["pets"]) {
      std::size_t idx = pet["index"].get<std::size_t>();
      bool fainted = pet.value("fainted", false);
      if (!fainted && idx != activeIndex)
        return idx;
    }
    return 0;
  };

  while (true) {
    auto now = std::chrono::steady_clock::now();
    bool needRender = false;

    if (now - lastFetch >= std::chrono::milliseconds(300)) {
      lastFetch = now;
      auto stateResp = client.get("/state?room=" + room +
                                  "&playerId=" + std::to_string(playerId));
      if (stateResp.status != 200) {
        std::cerr << "State error: " << stateResp.body << "\n";
        exitAltScreen();
        disableRawMode(guard);
        return 1;
      }
      nlohmann::json next =
          nlohmann::json::parse(stateResp.body, nullptr, false);
      if (next.is_discarded()) {
        std::cerr << "Invalid state response.\n";
        exitAltScreen();
        disableRawMode(guard);
        return 1;
      }
      state = std::move(next);
      if (state.value("battleOver", false)) {
        exitAltScreen();
        disableRawMode(guard);
        std::cout << "Battle ended. Winner=" << state.value("winner", 0)
                  << " Reason=" << state.value("reason", "") << "\n";
        client.post("/leave", {{"room", room}, {"playerId", playerId}});
        break;
      }

      pending = state.value("pending", "wait");
      if (pending != lastPending) {
        lastPending = pending;
        actionSubmitted = false;
        if (pending == "choose_action" || pending == "force_switch") {
          actionDeadline = std::chrono::steady_clock::now() + std::chrono::seconds(10);
          actionActive = true;
          message.clear();
          if (pending == "force_switch") {
            rightIdx = 1;
            leftIdx = 0;
            focusLeft = true;
          } else {
            rightIdx = 0;
            leftIdx = 0;
            focusLeft = false;
          }
        } else {
          actionActive = false;
          message.clear();
        }
      }
      needRender = true;
    }

    if (pending == "wait") {
      message = "等待对手...";
      needRender = true;
      std::this_thread::sleep_for(std::chrono::milliseconds(60));
    }

    int remaining = -1;
    if (actionActive) {
      auto diff = std::chrono::duration_cast<std::chrono::seconds>(
          actionDeadline - std::chrono::steady_clock::now())
                      .count();
      remaining = static_cast<int>(diff);
      if (remaining < 0)
        remaining = 0;
    }
    if (remaining != lastRemaining) {
      lastRemaining = remaining;
      needRender = true;
    }

    if (actionActive && !actionSubmitted && remaining == 0) {
      if (pending == "force_switch") {
        std::size_t idx = chooseAutoSwitch(state);
        client.post("/action", {{"room", room},
                                {"playerId", playerId},
                                {"type", "switch"},
                                {"index", idx}});
      } else {
        ActionData action = randomSkillFromState(state);
        nlohmann::json payload;
        payload["room"] = room;
        payload["playerId"] = playerId;
        if (action.type == ActionType::Skill) {
          payload["type"] = "skill";
          payload["skillId"] = action.skillId;
        } else {
          payload["type"] = "stay";
        }
        client.post("/action", payload);
      }
      actionSubmitted = true;
      message = "已提交，等待对手...";
      needRender = true;
    }

    NavKey key = readNavKey(120);
    if (pending == "choose_action" || pending == "force_switch") {
      bool forceSwitch = (pending == "force_switch");
      if (!forceSwitch) {
        if (key == NavKey::Left) {
          focusLeft = true;
          leftIdx = 0;
          needRender = true;
        } else if (key == NavKey::Right) {
          focusLeft = false;
          rightIdx = 0;
          needRender = true;
        }
      }
      if (key == NavKey::Up) {
        if (focusLeft) {
          leftIdx = std::max(0, leftIdx - 1);
        } else {
          rightIdx = std::max(0, rightIdx - 1);
          leftIdx = 0;
        }
        needRender = true;
      } else if (key == NavKey::Down) {
        if (focusLeft) {
          leftIdx = leftIdx + 1;
        } else {
          rightIdx = std::min(static_cast<int>(rightItems.size()) - 1, rightIdx + 1);
          leftIdx = 0;
        }
        needRender = true;
      } else if (key == NavKey::Enter && !actionSubmitted) {
        if (forceSwitch) {
          auto items = buildSwitchItems(state);
          if (leftIdx < 0 || static_cast<std::size_t>(leftIdx) >= items.size()) {
            message = "请选择可替换的宠物";
            needRender = true;
          } else if (!items[static_cast<std::size_t>(leftIdx)].selectable) {
            message = "当前宠物不可替换";
            needRender = true;
          } else {
            client.post("/action", {{"room", room},
                                    {"playerId", playerId},
                                    {"type", "switch"},
                                    {"index", items[static_cast<std::size_t>(leftIdx)].switchIndex}});
            actionSubmitted = true;
            message = "已提交，等待对手...";
            needRender = true;
          }
        } else if (!focusLeft) {
          if (rightIdx == 0) {
            focusLeft = true;
            leftIdx = 0;
            needRender = true;
          } else if (rightIdx == 1) {
            focusLeft = true;
            leftIdx = 0;
            needRender = true;
          } else if (rightIdx == 2) {
            client.post("/action", {{"room", room},
                                    {"playerId", playerId},
                                    {"type", "flee"}});
            actionSubmitted = true;
            message = "已提交，等待对手...";
            needRender = true;
          } else if (rightIdx == 3) {
            client.post("/action", {{"room", room},
                                    {"playerId", playerId},
                                    {"type", "stay"}});
            actionSubmitted = true;
            message = "已提交，等待对手...";
            needRender = true;
          }
        } else {
          if (rightIdx == 0) {
            auto items = buildSkillItems(state);
            if (leftIdx < 0 || static_cast<std::size_t>(leftIdx) >= items.size()) {
              message = "请选择技能";
              needRender = true;
            } else if (!items[static_cast<std::size_t>(leftIdx)].selectable) {
              message = "该技能不可用";
              needRender = true;
            } else {
              client.post("/action", {{"room", room},
                                      {"playerId", playerId},
                                      {"type", "skill"},
                                      {"skillId", items[static_cast<std::size_t>(leftIdx)].skillId}});
              actionSubmitted = true;
              message = "已提交，等待对手...";
              needRender = true;
            }
          } else if (rightIdx == 1) {
            auto items = buildSwitchItems(state);
            if (leftIdx < 0 || static_cast<std::size_t>(leftIdx) >= items.size()) {
              message = "请选择可替换的宠物";
              needRender = true;
            } else if (!items[static_cast<std::size_t>(leftIdx)].selectable) {
              message = "当前宠物不可替换";
              needRender = true;
            } else {
              client.post("/action", {{"room", room},
                                      {"playerId", playerId},
                                      {"type", "switch"},
                                      {"index", items[static_cast<std::size_t>(leftIdx)].switchIndex}});
              actionSubmitted = true;
              message = "已提交，等待对手...";
              needRender = true;
            }
          }
        }
      }
    }

    if (needRender && !state.is_null()) {
      std::cout << "\033[H\033[2J";
      renderBattleInfo(state, remaining);
      std::vector<LeftItem> leftItems;
      if (pending == "force_switch" || rightIdx == 1) {
        leftItems = buildSwitchItems(state);
      } else {
        leftItems = buildSkillItems(state);
      }
      if (leftIdx < 0)
        leftIdx = 0;
      if (!leftItems.empty() && leftIdx >= static_cast<int>(leftItems.size()))
        leftIdx = static_cast<int>(leftItems.size()) - 1;
      renderBattleMenu(leftItems, rightItems, leftIdx, rightIdx, focusLeft, message);
    }
  }
  return 0;
#else
  (void)client;
  (void)room;
  (void)playerId;
  return 0;
#endif
}

int runRoomLobby(HttpClient &client, const std::string &room, int playerId) {
#ifndef _WIN32
  TermiosGuard guard;
  if (!enableRawMode(guard)) {
    std::vector<std::string> options = {"ready", "unready", "leave", "refresh"};
    while (true) {
      auto roomResp = client.get("/room?name=" + room);
      if (roomResp.status != 200) {
        std::cerr << "Room error: " << roomResp.body << "\n";
        return 0;
      }
      nlohmann::json roomJson =
          nlohmann::json::parse(roomResp.body, nullptr, false);
      if (roomJson.is_discarded()) {
        std::cerr << "Room response invalid.\n";
        return 0;
      }
      if (roomJson.value("battleStarted", false)) {
        std::string role = playerId == 1 ? "玩家1" : "玩家2";
        std::cout << "***进入房间（" << role << "）\n";
        return runBattle(client, room, playerId);
      }
      printRoomInfo(roomJson);
      int choice = selectMenu("Room actions (Up/Down + Enter, q to cancel)", options, 0);
      if (choice < 0)
        continue;
      const std::string &cmd = options[static_cast<std::size_t>(choice)];
      if (cmd == "ready") {
        client.post("/ready",
                    {{"room", room}, {"playerId", playerId}, {"ready", true}});
      } else if (cmd == "unready") {
        client.post("/ready",
                    {{"room", room}, {"playerId", playerId}, {"ready", false}});
      } else if (cmd == "leave") {
        client.post("/leave", {{"room", room}, {"playerId", playerId}});
        return 0;
      }
    }
  }

  enterAltScreen();

  std::unordered_map<std::string, std::string> prevPlayers;
  std::unordered_set<std::string> prevSpectators;
  bool seenSnapshot = false;
  int selected = 0;
  std::vector<std::string> options = {"ready", "unready", "leave", "refresh"};
  nlohmann::json roomJson = nlohmann::json::object();
  std::vector<std::string> messageLog;
  auto lastFetch = std::chrono::steady_clock::now() - std::chrono::seconds(1);

  while (true) {
    auto now = std::chrono::steady_clock::now();
    bool needRender = false;
    std::vector<std::string> messages;

    if (now - lastFetch >= std::chrono::milliseconds(500)) {
      lastFetch = now;
      auto roomResp = client.get("/room?name=" + room);
      if (roomResp.status != 200) {
        std::cerr << "Room error: " << roomResp.body << "\n";
        exitAltScreen();
        disableRawMode(guard);
        return 0;
      }
      nlohmann::json nextRoom =
          nlohmann::json::parse(roomResp.body, nullptr, false);
      if (nextRoom.is_discarded()) {
        std::cerr << "Room response invalid.\n";
        exitAltScreen();
        disableRawMode(guard);
        return 0;
      }
      roomJson = std::move(nextRoom);
      if (roomJson.value("battleStarted", false)) {
        exitAltScreen();
        disableRawMode(guard);
        return runBattle(client, room, playerId);
      }

      auto players = collectPlayerRoles(roomJson);
      if (seenSnapshot) {
        for (const auto &kv : players) {
          if (prevPlayers.find(kv.first) == prevPlayers.end()) {
            messages.push_back(kv.first + "进入了房间（" + kv.second + "）");
          }
        }
        for (const auto &kv : prevPlayers) {
          if (players.find(kv.first) == players.end()) {
            messages.push_back(kv.first + "离开了房间（" + kv.second + "）");
          }
        }
        std::unordered_set<std::string> curSpecs;
        collectSpectatorNames(roomJson, curSpecs);
        for (const auto &name : curSpecs) {
          if (prevSpectators.find(name) == prevSpectators.end()) {
            messages.push_back(name + "进入了房间（观众）");
          }
        }
        for (const auto &name : prevSpectators) {
          if (curSpecs.find(name) == curSpecs.end()) {
            messages.push_back(name + "离开了房间（观众）");
          }
        }
        prevSpectators = std::move(curSpecs);
      } else {
        collectSpectatorNames(roomJson, prevSpectators);
        std::string role = playerId == 1 ? "玩家1" : "玩家2";
        messages.push_back("进入了房间（" + role + "）");
        seenSnapshot = true;
      }
      prevPlayers = std::move(players);
      needRender = true;
    }

    NavKey key = readNavKey(200);
    if (key == NavKey::Up) {
      selected = (selected - 1 + static_cast<int>(options.size())) % static_cast<int>(options.size());
      needRender = true;
    } else if (key == NavKey::Down) {
      selected = (selected + 1) % static_cast<int>(options.size());
      needRender = true;
    } else if (key == NavKey::Enter) {
      const std::string &cmd = options[static_cast<std::size_t>(selected)];
      if (cmd == "ready") {
        client.post("/ready",
                    {{"room", room}, {"playerId", playerId}, {"ready", true}});
      } else if (cmd == "unready") {
        client.post("/ready",
                    {{"room", room}, {"playerId", playerId}, {"ready", false}});
      } else if (cmd == "leave") {
        client.post("/leave", {{"room", room}, {"playerId", playerId}});
        exitAltScreen();
        disableRawMode(guard);
        return 0;
      }
      needRender = true;
    } else if (key == NavKey::Quit) {
      client.post("/leave", {{"room", room}, {"playerId", playerId}});
      exitAltScreen();
      disableRawMode(guard);
      return 0;
    }

    if (!messages.empty()) {
      for (const auto &msg : messages) {
        messageLog.push_back(msg);
      }
      if (messageLog.size() > 5) {
        messageLog.erase(messageLog.begin(), messageLog.end() - 5);
      }
    }

    if (needRender && !roomJson.is_null()) {
      renderRoomScreen(roomJson, messageLog, options, selected);
    }
  }
#else
  (void)client;
  (void)room;
  (void)playerId;
  return 0;
#endif
}

int runSpectator(HttpClient &client, const std::string &room,
                 const std::string &name) {
#ifndef _WIN32
  TermiosGuard guard;
  if (!enableRawMode(guard)) {
    std::cout << "Spectating room " << room << ". Type 'leave' to exit.\n";
    int lastTurnShown = -1;
    bool printedWaiting = false;
    while (true) {
      auto stateResp =
          client.get("/state?room=" + room + "&spectator=1&name=" + name);
      if (stateResp.status != 200) {
        std::cerr << "Spectate error: " << stateResp.body << "\n";
        break;
      }
      nlohmann::json state =
          nlohmann::json::parse(stateResp.body, nullptr, false);
      if (state.is_discarded()) {
        std::cerr << "Invalid spectator state.\n";
        break;
      }

      if (state.value("status", "") == "waiting") {
        if (!printedWaiting) {
          std::cout << "Waiting for battle to start...\n";
          printedWaiting = true;
        }
        if (state.contains("room")) {
          printRoomInfo(state["room"]);
        }
      } else {
        printedWaiting = false;
        if (state.contains("lastActions") && state["lastActions"].is_object()) {
          int turn = state["lastActions"].value("turn", -1);
          if (turn != lastTurnShown) {
            lastTurnShown = turn;
            printLastActions(state["lastActions"]);
          }
        }
        if (state.value("battleOver", false)) {
          std::cout << "Battle ended. Winner=" << state.value("winner", 0)
                    << " Reason=" << state.value("reason", "") << "\n";
          break;
        }
      }

      auto line = readLineWithTimeout(std::chrono::milliseconds(200));
      if (line && *line == "leave") {
        break;
      }
    }
    client.post("/leave", {{"room", room}, {"name", name}});
    return 0;
  }

  enterAltScreen();
  int lastTurnShown = -1;
  bool printedWaiting = false;
  std::unordered_map<std::string, std::string> prevPlayers;
  std::unordered_set<std::string> prevSpectators;
  bool seenSnapshot = false;
  std::vector<std::string> messageLog;
  nlohmann::json roomInfo = nlohmann::json::object();
  auto lastFetch = std::chrono::steady_clock::now() - std::chrono::seconds(1);

  while (true) {
    auto now = std::chrono::steady_clock::now();
    bool needRender = false;
    std::vector<std::string> messages;

    if (now - lastFetch >= std::chrono::milliseconds(500)) {
      lastFetch = now;
      auto stateResp =
          client.get("/state?room=" + room + "&spectator=1&name=" + name);
      if (stateResp.status != 200) {
        std::cerr << "Spectate error: " << stateResp.body << "\n";
        exitAltScreen();
        disableRawMode(guard);
        break;
      }
      nlohmann::json state =
          nlohmann::json::parse(stateResp.body, nullptr, false);
      if (state.is_discarded()) {
        std::cerr << "Invalid spectator state.\n";
        exitAltScreen();
        disableRawMode(guard);
        break;
      }

      if (state.value("status", "") == "waiting") {
        if (!printedWaiting) {
          messages.push_back("Waiting for battle to start...");
          printedWaiting = true;
        }
        if (state.contains("room")) {
          roomInfo = state["room"];
          auto players = collectPlayerRoles(roomInfo);
          if (seenSnapshot) {
            for (const auto &kv : players) {
              if (prevPlayers.find(kv.first) == prevPlayers.end()) {
                messages.push_back(kv.first + "进入了房间（" + kv.second + "）");
              }
            }
            for (const auto &kv : prevPlayers) {
              if (players.find(kv.first) == players.end()) {
                messages.push_back(kv.first + "离开了房间（" + kv.second + "）");
              }
            }
            std::unordered_set<std::string> curSpecs;
            collectSpectatorNames(roomInfo, curSpecs);
            for (const auto &nameItem : curSpecs) {
              if (prevSpectators.find(nameItem) == prevSpectators.end()) {
                messages.push_back(nameItem + "进入了房间（观众）");
              }
            }
            for (const auto &nameItem : prevSpectators) {
              if (curSpecs.find(nameItem) == curSpecs.end()) {
                messages.push_back(nameItem + "离开了房间（观众）");
              }
            }
            prevSpectators = std::move(curSpecs);
          } else {
            collectSpectatorNames(roomInfo, prevSpectators);
            seenSnapshot = true;
          }
          prevPlayers = std::move(players);
        }
      } else {
        printedWaiting = false;
        if (state.contains("lastActions") && state["lastActions"].is_object()) {
          int turn = state["lastActions"].value("turn", -1);
          if (turn != lastTurnShown) {
            lastTurnShown = turn;
            std::ostringstream oss;
            oss << "Turn " << turn << " actions: ";
            auto act = [&](const nlohmann::json &a) {
              std::string type = a.value("type", "stay");
              if (type == "skill") {
                oss << "skill(" << a.value("skillId", 0) << ")";
              } else if (type == "switch") {
                oss << "switch(" << a.value("index", 0) << ")";
              } else {
                oss << type;
              }
            };
            oss << "P1 ";
            act(state["lastActions"].value("p1", nlohmann::json::object()));
            oss << " | P2 ";
            act(state["lastActions"].value("p2", nlohmann::json::object()));
            messages.push_back(oss.str());
          }
        }
        if (state.value("battleOver", false)) {
          messages.push_back("Battle ended. Winner=" +
                             std::to_string(state.value("winner", 0)) +
                             " Reason=" + state.value("reason", ""));
          needRender = true;
          if (!messages.empty()) {
            for (const auto &msg : messages) {
              messageLog.push_back(msg);
            }
            if (messageLog.size() > 8) {
              messageLog.erase(messageLog.begin(), messageLog.end() - 8);
            }
          }
          if (!roomInfo.is_null()) {
            std::vector<std::string> opts = {"leave"};
            renderRoomScreen(roomInfo, messageLog, opts, 0);
          }
          break;
        }
      }

      needRender = true;
    }

    if (!messages.empty()) {
      for (const auto &msg : messages) {
        messageLog.push_back(msg);
      }
      if (messageLog.size() > 8) {
        messageLog.erase(messageLog.begin(), messageLog.end() - 8);
      }
    }

    if (needRender) {
      if (!roomInfo.is_null()) {
        std::vector<std::string> opts = {"leave"};
        renderRoomScreen(roomInfo, messageLog, opts, 0);
      } else {
        std::cout << "\033[H\033[2J";
        std::cout << "Spectating room " << room << "\n";
        for (const auto &msg : messageLog) {
          std::cout << msg << "\n";
        }
        std::cout << "\nActions:\n> leave\n" << std::flush;
      }
    }

    NavKey key = readNavKey(200);
    if (key == NavKey::Enter || key == NavKey::Quit) {
      break;
    }
  }

  exitAltScreen();
  disableRawMode(guard);
  client.post("/leave", {{"room", room}, {"name", name}});
  return 0;
#else
  (void)client;
  (void)room;
  (void)name;
  return 0;
#endif
}
} // namespace

int runClient(const std::string &host, int port) {
  HttpClient client(host, port);

  std::string name = promptLine("Enter your name: ");

  while (true) {
    std::vector<std::string> lobbyOptions = {"list", "create", "join", "random",
                                             "spectate", "quit"};
    int choice = selectMenu("Lobby (Up/Down + Enter, q to cancel)", lobbyOptions, 0);
    if (choice < 0)
      continue;
    const std::string &cmd = lobbyOptions[static_cast<std::size_t>(choice)];

    if (cmd == "list") {
      auto resp = client.get("/rooms");
      if (resp.status != 200) {
        std::cerr << "List failed: " << resp.body << "\n";
        continue;
      }
      nlohmann::json data = nlohmann::json::parse(resp.body, nullptr, false);
      if (data.is_discarded()) {
        std::cerr << "Invalid list response.\n";
        continue;
      }
      printRooms(data.value("rooms", nlohmann::json::array()));
      continue;
    }

    if (cmd == "create") {
      std::string room = promptLine("Room name: ");
      auto resp = client.post("/create", {{"room", room}, {"name", name}});
      if (resp.status == 409) {
        std::cerr << kRed << "Room already exists." << kReset << "\n";
        continue;
      }
      if (resp.status != 200) {
        std::cerr << "Create failed: " << resp.body << "\n";
        continue;
      }
      nlohmann::json joinJson =
          nlohmann::json::parse(resp.body, nullptr, false);
      if (joinJson.is_discarded()) {
        std::cerr << "Create response invalid.\n";
        continue;
      }
      int playerId = joinJson.value("playerId", 0);
      std::string roomName = joinJson.value("room", "");
      if (playerId <= 0 || roomName.empty()) {
        std::cerr << "Create failed: " << resp.body << "\n";
        continue;
      }
      std::cout << "Created room " << roomName << " as player " << playerId
                << "\n";
      runRoomLobby(client, roomName, playerId);
      continue;
    }

    if (cmd == "join") {
      auto listResp = client.get("/rooms");
      if (listResp.status != 200) {
        std::cerr << "List failed: " << listResp.body << "\n";
        continue;
      }
      nlohmann::json listJson =
          nlohmann::json::parse(listResp.body, nullptr, false);
      if (listJson.is_discarded()) {
        std::cerr << "Invalid list response.\n";
        continue;
      }
      auto roomOpt = selectRoomFromList(
          listJson.value("rooms", nlohmann::json::array()),
          "Select room to join (Up/Down + Enter, q to cancel)");
      if (!roomOpt.has_value())
        continue;
      std::string room = *roomOpt;
      auto resp = client.post("/join", {{"room", room}, {"name", name}});
      if (resp.status != 200) {
        std::cerr << "Join failed: " << resp.body << "\n";
        continue;
      }
      nlohmann::json joinJson =
          nlohmann::json::parse(resp.body, nullptr, false);
      if (joinJson.is_discarded()) {
        std::cerr << "Join response invalid.\n";
        continue;
      }
      int playerId = joinJson.value("playerId", 0);
      std::string roomName = joinJson.value("room", "");
      if (playerId <= 0 || roomName.empty()) {
        std::cerr << "Join failed: " << resp.body << "\n";
        continue;
      }
      std::cout << "Joined room " << roomName << " as player " << playerId
                << "\n";
      runRoomLobby(client, roomName, playerId);
      continue;
    }

    if (cmd == "random") {
      auto resp = client.post("/join_random", {{"name", name}});
      if (resp.status != 200) {
        std::cerr << "Random join failed: " << resp.body << "\n";
        continue;
      }
      nlohmann::json joinJson =
          nlohmann::json::parse(resp.body, nullptr, false);
      if (joinJson.is_discarded()) {
        std::cerr << "Random join response invalid.\n";
        continue;
      }
      int playerId = joinJson.value("playerId", 0);
      std::string roomName = joinJson.value("room", "");
      if (playerId <= 0 || roomName.empty()) {
        std::cerr << "Random join failed: " << resp.body << "\n";
        continue;
      }
      std::cout << "Joined room " << roomName << " as player " << playerId
                << "\n";
      runRoomLobby(client, roomName, playerId);
      continue;
    }

    if (cmd == "spectate") {
      auto listResp = client.get("/rooms");
      if (listResp.status != 200) {
        std::cerr << "List failed: " << listResp.body << "\n";
        continue;
      }
      nlohmann::json listJson =
          nlohmann::json::parse(listResp.body, nullptr, false);
      if (listJson.is_discarded()) {
        std::cerr << "Invalid list response.\n";
        continue;
      }
      auto roomOpt = selectRoomFromList(
          listJson.value("rooms", nlohmann::json::array()),
          "Select room to spectate (Up/Down + Enter, q to cancel)");
      if (!roomOpt.has_value())
        continue;
      std::string room = *roomOpt;
      auto resp = client.post("/spectate", {{"room", room}, {"name", name}});
      if (resp.status != 200) {
        std::cerr << "Spectate failed: " << resp.body << "\n";
        continue;
      }
      nlohmann::json specJson =
          nlohmann::json::parse(resp.body, nullptr, false);
      if (specJson.is_discarded()) {
        std::cerr << "Spectate response invalid.\n";
        continue;
      }
      std::string roomName = specJson.value("room", "");
      if (roomName.empty()) {
        std::cerr << "Spectate failed: " << resp.body << "\n";
        continue;
      }
      runSpectator(client, roomName, name);
      continue;
    }

    if (cmd == "quit") {
      return 0;
    }
  }
}
