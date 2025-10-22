#include "Database.h"

explicit Database::Database(const std::string& dbPath) {
    if(sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
        throw std::runtime_error("Failed to open DATABASE :" + std::string(sqlite3_errmsg(db)));
    }

    exec("PRAGMA jornal_mode = WAL;");
    exec("PRAGMA synchronous = NORMAL;");
    exec("PRAGMA cache_size = 10000;");
}

Database::~Database() {
    if(db) sqlite3_close(db);
}

void Database::exec(const std::string& sql) {
    char* errMsg = nullptr;
    if(sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &errMsg) != SQLITE_OK) {
        std::string msg(errMsg);
        sqlite3_free(errMsg);
        throw std::runtime_error("SQLite exec error: " + msg);
    }
}

void Database::execute(const std::string& sql, const std::vector<std::string>& params) {
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("SQLite Prepare failed: " + std::string(sqlite3_errmsg(db)));
    }

    bindParams(stmt, params);
    stepAndFinalize(stmt);
}