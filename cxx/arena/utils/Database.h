#pragma once

/*
需安装libsqlite3-dev包

一键脚本：
    chmod +x init.sh && ./init.sh
debian类linux指令：
    sudo apt install libsqlite3-dev
红帽不知道喵

*/
#include <sqlite3.h>
#include <string>
#include <vector>
#include <unordered_map>
#include <stdexcept>



//否则，请引用
//#include "sqlite/sqlite3.h"


class Database {
public:
    //构造与析构
    explicit Database(const std::string& dbPath);
    ~Database();

    // 执行无返回值的 SQL (建表、更新、删除等)
    void exec(const std::string& sql);
    // 执行带参数的语句 (如 INSERT/UPDATE)
    void execute(const std::string& sql, const std::vector<std::string>& params = {});
    // 查询返回多行结果
    std::vector<std::unordered_map<std::string, std::string>> query(
        const std::string& sql,
        const std::vector<std::string>& params = {}
    ) {};
    // 开始事务
    void begin() { exec("BEGIN TRANSACTION;"); }
    void commit() { exec("COMMIT;"); }
    void rollback() { exec("ROLLBACK;"); }

private:
    sqlite3* db = nullptr;
    void bindParams(sqlite3_stmt* stmt, const std::vector<std::string>& params) {};
    void stepAndFinalize(sqlite3_stmt* stmt) {};
};