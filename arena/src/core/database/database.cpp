#include "database.h"

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

std::vector<std::unordered_map<std::string, std::string>> Database::query(
    const std::string& sql,
    const std::vector<std::string>& params
) {
    sqlite3_stmt* stmt;
    if(sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("SQLite Prepare failed: " + std::string(sqlite3_errmsg(db)));
    }

    bindParams(stmt, params);

    std::vector<std::unordered_map<std::string, std::string>> rows;
    int cols = sqlite3_column_count(stmt);

    while(sqlite3_step(stmt) == SQLITE_ROW) {
        std::unordered_map<std::string, std::string> row;
        for(int i = 0; i < cols; ++i) {
            const char* name = sqlite3_column_name(stmt, i);
            const unsigned char* value = sqlite3_column_text(stmt, i);
            row[name] = value ? reinterpret_cast<const char*>(value) : "";
        }
        rows.push_back(row);
    }
    
    sqlite3_finalize(stmt);
    return rows;
}

void Database::bindParams(sqlite3_stmt* stmt, const std::vector<std::string>& params) {
    for(size_t i = 0; i < params.size(); ++i) {
        sqlite3_bind_text(stmt, static_cast<int>(i + 1), params[i].c_str(), -1, SQLITE_STATIC);
    }
}

void Database::stepAndFinalize(sqlite3_stmt* stmt) {
    if(sqlite3_step(stmt) != SQLITE_DONE) {
        std::string msg = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("SQLite execute error: " + msg);
    }
    sqlite3_finalize(stmt);
}