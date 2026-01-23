#include "data_loader.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <optional>
#include <sstream>

#include <database/database.h>
#include <entity/EVData.h>
#include <entity/IVData.h>
#include <entity/Nature.h>
#include <entity/Pet.h>
#include <entity/Attr.h>

namespace {
namespace fs = std::filesystem;

bool isAssetsDir(const fs::path& dir) {
    return fs::exists(dir / "pets") && fs::exists(dir / "skills");
}

std::optional<fs::path> findAssetsRoot() {
    fs::path current = fs::current_path();
    for (fs::path dir = current; !dir.empty(); dir = dir.parent_path()) {
        fs::path direct = dir / "assets";
        if (isAssetsDir(direct)) return direct;
        fs::path nested = dir / "arena" / "assets";
        if (isAssetsDir(nested)) return nested;
        if (dir == dir.parent_path()) break;
    }
    return std::nullopt;
}

fs::path resolveAssetPath(const fs::path& requested, const std::optional<fs::path>& assetsRoot) {
    if (requested.empty() || requested.is_absolute() || fs::exists(requested) || !assetsRoot) {
        return requested;
    }

    const std::string reqStr = requested.generic_string();
    const std::string marker = "assets/";
    auto pos = reqStr.find(marker);
    if (pos != std::string::npos) {
        const std::string suffix = reqStr.substr(pos + marker.size());
        return *assetsRoot / suffix;
    }
    return *assetsRoot / requested;
}

bool parseAttr(const std::string& raw, AttrType& out) {
    if (auto it = AttrFromEN.find(raw); it != AttrFromEN.end()) {
        out = it->second;
        return true;
    }
    if (auto it = AttrFromZH.find(raw); it != AttrFromZH.end()) {
        out = it->second;
        return true;
    }
    if (raw == "None" || raw.empty()) {
        out = AttrType::None;
        return true;
    }
    return false;
}

bool parseInt(const std::unordered_map<std::string, std::string>& row, const std::string& key, int& out) {
    auto it = row.find(key);
    if (it == row.end()) return false;
    try {
        out = std::stoi(it->second);
        return true;
    } catch (...) {
        return false;
    }
}

std::string getStr(const std::unordered_map<std::string, std::string>& row, const std::string& key) {
    auto it = row.find(key);
    if (it == row.end()) return {};
    return it->second;
}
} // namespace

bool loadDataStore(const std::string& petsDbPath, const std::string& skillsDir, DataStore& out,
                   std::string* error) {
    out.pets.clear();
    out.skills = SkillRegistry{};

    auto assetsRoot = findAssetsRoot();
    fs::path resolvedPetsDbPath = resolveAssetPath(petsDbPath, assetsRoot);
    fs::path resolvedSkillsDir = resolveAssetPath(skillsDir, assetsRoot);

    if (!out.skills.loadFromDirectory(resolvedSkillsDir.string(), error)) {
        if (error && error->empty()) {
            *error = "failed to load skills from directory: " + resolvedSkillsDir.string();
        }
        return false;
    }

    Database db(resolvedPetsDbPath.string());
    auto rows = db.query("SELECT id, name, attr1, attr2, bEne, bAtk, bDef, bSpA, bSpD, bSpe FROM pets");
    if (rows.empty()) {
        if (error) *error = "no pets found in database";
        return false;
    }

    std::unordered_map<int, std::vector<int>> skillMap;
    auto skillRows = db.query("SELECT pet_id, skill_id FROM pet_skills");
    for (const auto& row : skillRows) {
        int petId = 0;
        int skillId = 0;
        if (!parseInt(row, "pet_id", petId) || !parseInt(row, "skill_id", skillId)) {
            continue;
        }
        skillMap[petId].push_back(skillId);
    }

    for (const auto& row : rows) {
        int id = 0;
        int bEne = 0, bAtk = 0, bDef = 0, bSpA = 0, bSpD = 0, bSpe = 0;
        if (!parseInt(row, "id", id)) {
            continue;
        }
        if (!parseInt(row, "bEne", bEne) || !parseInt(row, "bAtk", bAtk) || !parseInt(row, "bDef", bDef) ||
            !parseInt(row, "bSpA", bSpA) || !parseInt(row, "bSpD", bSpD) || !parseInt(row, "bSpe", bSpe)) {
            continue;
        }

        std::string name = getStr(row, "name");
        std::string attr1Raw = getStr(row, "attr1");
        std::string attr2Raw = getStr(row, "attr2");

        AttrType attr1 = AttrType::None;
        AttrType attr2 = AttrType::None;
        if (!parseAttr(attr1Raw, attr1) || !parseAttr(attr2Raw, attr2)) {
            continue;
        }

        BS bs(bEne, bAtk, bDef, bSpA, bSpD, bSpe);
        auto species = std::make_shared<Species>(id, name, std::array<AttrType, 2>{ attr1, attr2 }, bs);

        PetTemplate templ;
        templ.species = std::move(species);
        if (auto it = skillMap.find(id); it != skillMap.end()) {
            templ.learnableSkillIds = it->second;
        }

        out.pets.emplace(id, std::move(templ));
    }

    if (out.pets.empty()) {
        if (error) *error = "failed to load any pet templates";
        return false;
    }

    return true;
}

std::unique_ptr<Pet> createPetFromTemplate(const PetTemplate& templ, const SkillRegistry& registry, int level) {
    if (!templ.species) return nullptr;

    auto pet = std::make_unique<Pet>(const_cast<Species*>(templ.species.get()), IVData{}, EVData{});
    pet->calcRealStat(templ.species->baseStats(), IVData{}, EVData{}, NatureType::Hardy, level);

    pet->setLearnableSkills(templ.learnableSkillIds);

    int filled = 0;
    for (int skillId : templ.learnableSkillIds) {
        if (filled >= static_cast<int>(Pet::kMaxSkillSlots)) break;
        if (pet->configureSkill(skillId, registry)) {
            ++filled;
        }
    }

    return pet;
}
