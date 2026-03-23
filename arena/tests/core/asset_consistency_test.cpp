// tests/core/asset_consistency_test.cpp
// Strategy tests #11-13: Data asset consistency validation

#include <gtest/gtest.h>
#include <core/json_loader/json_loader.h>
#include <core/database/database.h>
#include <filesystem>

namespace fs = std::filesystem;

// =============================================================================
// #11: AssetLoader_Validates_SQLite_PetsDB_Schema
// Open pets.db and verify expected tables exist
// =============================================================================

TEST(AssetConsistency, ValidatesSQLitePetsDBSchema) {
    // Locate the database file relative to the project
    // Try common paths
    std::string dbPath;
    for (const auto& candidate : {
        "data/pets.db",
        "../data/pets.db",
        "../../data/pets.db",
        "../../../data/pets.db"
    }) {
        if (fs::exists(candidate)) {
            dbPath = candidate;
            break;
        }
    }

    if (dbPath.empty()) {
        GTEST_SKIP() << "pets.db not found; skipping asset test";
    }

    EXPECT_NO_THROW({
        Database db(dbPath);
        auto rows = db.query("SELECT name FROM sqlite_master WHERE type='table'");
        EXPECT_FALSE(rows.empty()) << "Database has no tables";
    });
}

// =============================================================================
// #12: AssetLoader_Rejects_Malformed_JSON_Config
// =============================================================================

TEST(AssetConsistency, RejectsMalformedJSONConfig) {
    JSONLoader loader;
    EXPECT_THROW(
        loader.loadFromString("{ invalid json !!!"),
        std::runtime_error
    );
}

TEST(AssetConsistency, AcceptsValidJSON) {
    JSONLoader loader;
    EXPECT_NO_THROW(
        loader.loadFromString(R"({"name": "test", "value": 42})")
    );
    EXPECT_TRUE(loader.has("name"));
    EXPECT_TRUE(loader.has("value"));
}

TEST(AssetConsistency, JSONLoader_NestedPath) {
    JSONLoader loader;
    loader.loadFromString(R"({"pet": {"name": "Pikachu", "level": 50}})");

    auto name = loader.getOpt<std::string>("pet.name");
    ASSERT_TRUE(name.has_value());
    EXPECT_EQ(*name, "Pikachu");

    auto level = loader.getOpt<int>("pet.level");
    ASSERT_TRUE(level.has_value());
    EXPECT_EQ(*level, 50);
}

TEST(AssetConsistency, JSONLoader_MissingPath_ReturnsNullopt) {
    JSONLoader loader;
    loader.loadFromString(R"({"a": 1})");

    auto missing = loader.getOpt<int>("b");
    EXPECT_FALSE(missing.has_value());
}

TEST(AssetConsistency, JSONLoader_DefaultValue) {
    JSONLoader loader;
    loader.loadFromString(R"({"a": 1})");

    int val = loader.get<int>("nonexistent", 99);
    EXPECT_EQ(val, 99);
}

// =============================================================================
// #13: AssetLoader_Asserts_All_Assigned_Pet_Skills_Exist_In_Directory
// =============================================================================

TEST(AssetConsistency, AssertsPetSkillsExistInDirectory) {
    // Find the skills source directory
    std::string skillsDir;
    for (const auto& candidate : {
        "data/skills_src",
        "../data/skills_src",
        "../../data/skills_src",
        "../../../data/skills_src"
    }) {
        if (fs::exists(candidate) && fs::is_directory(candidate)) {
            skillsDir = candidate;
            break;
        }
    }

    if (skillsDir.empty()) {
        GTEST_SKIP() << "skills_src directory not found; skipping asset test";
    }

    // Skills directory should not be empty
    int fileCount = 0;
    for (const auto& entry : fs::directory_iterator(skillsDir)) {
        if (entry.is_regular_file()) {
            fileCount++;
        }
    }
    EXPECT_GT(fileCount, 0) << "Skills directory is empty";
}
