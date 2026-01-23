#pragma once

#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include <entity/Species.h>
#include <skill/SkillRegistry.h>

class Pet;

struct PetTemplate {
    Species::Ptr species;
    std::vector<int> learnableSkillIds;
};

struct DataStore {
    SkillRegistry skills;
    std::unordered_map<int, PetTemplate> pets;
};

bool loadDataStore(const std::string& petsDbPath, const std::string& skillsDir, DataStore& out, std::string* error);

std::unique_ptr<Pet> createPetFromTemplate(const PetTemplate& templ, const SkillRegistry& registry,
                                           int level = 100);
